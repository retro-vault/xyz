        ; Boot esxDOS and execute C directly from the replacement ROM.
        ; Copy writable data, including the 48-byte disk-call gates.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module crt0
        .optsdcc -mz80 sdcccall(1)

        .globl  _main
        .globl  __exit
        .globl  _entry
        .globl  _zx_console_init
        .globl  __zx_rom_reset
        .globl  __zx_rom_startup
        .globl  s__BSS
        .globl  l__BSS
        .globl  s__INITIALIZED
        .globl  s__INITIALIZER
        .globl  l__INITIALIZER
        .globl  s__DATA
        .globl  l__DATA
        .globl  s__DATA_LOAD
        .globl  s__.data
        .globl  l__.data
        .globl  s__.data_LOAD
        .globl  s__.bss
        .globl  l__.bss

ZX_STACK_TOP    .equ    0xffff

        .area   _HEADER
        .area   _CODE
        .text
        .area   _CONST
        .rodata
        .area   _INITIALIZER
        .area   _GSINIT
        .area   _GSFINAL
        .area   _DATA
        .data
        .area   _INITIALIZED
        .area   _BSS
        .bss
        .area   _HEAP
        .area   _STACK

        .area   _HEADER

        ; __zx_rom_reset
        ; inputs: cold reset with divIDE and matching esxDOS firmware.
        ; outputs: firmware returns at 0001, then enters the ROM CRT.
        ; clobbers: all registers and flags during boot.
__zx_rom_reset::
        di
        ; Firmware resumes at 0001 with the 48K reset signature.
        xor     a
        jp      __zx_rom_startup
        .ds     3
        ; RST 08 keeps its three-byte Sinclair first instruction.
        ld      hl,(0x5c5d)
        .ds     5
        ; Firmware boot text may call the base-ROM RST 10 entry.
        ret
        .ds     39
        ; IRQ paging executes PUSH AF before entering esxDOS at 0039.
        ; Its return through the base ROM must balance that push.
        push    af
        pop     af
        ei
        reti
        .ds     41
        ; Preserve NMI paging's first opcode. The browser is
        ; unsupported.
        push    af
        .ds     20
        ; esxDOS reads the inline syscall number via base ROM at 007b.
        ld      a,(hl)
        ret
        .ds     131

        ; __zx_rom_startup
        ; inputs: esxDOS boot has returned through base-ROM address
        ; 0001.
        ; outputs: writable data/gates copied; enters _entry in ROM.
        ; clobbers: af, bc, de, hl and sp; interrupts remain disabled.
__zx_rom_startup::
        di
        ld      sp,#ZX_STACK_TOP
        ld      hl,#s__DATA_LOAD
        ld      de,#s__DATA
        ld      bc,#l__DATA
        ld      a,b
        or      c
        jr      z,.zxrom_no_data
        ldir
.zxrom_no_data:
        ld      hl,#s__.data_LOAD
        ld      de,#s__.data
        ld      bc,#l__.data
        ld      a,b
        or      c
        jr      z,.zxrom_no_gnu_data
        ldir
.zxrom_no_gnu_data:
        jp      _entry

        .area   _CODE

        ; _entry
        ; inputs: esxDOS boot and writable-data initialization are done.
        ; outputs: does not return; calls main then _exit.
        ; clobbers: all registers and flags; initializes SP and IY.
_entry::
        di
        ld      sp,#ZX_STACK_TOP
        ld      iy,#0x5c3a
        call    .zxesxdos_init
        call    _zx_console_init
        call    _main
        ex      de,hl
        jp      __exit

        .area   _GSINIT
.zxesxdos_init:
        ld      bc,#l__BSS
        ld      a,b
        or      c
        jr      z,.zxesxdos_no_bss
        ld      hl,#s__BSS
        ld      d,h
        ld      e,l
        inc     de
        ld      (hl),#0
        dec     bc
        ld      a,b
        or      c
        jr      z,.zxesxdos_no_bss
        ldir
.zxesxdos_no_bss:
        ; ELF C objects keep their uninitialized storage in .bss.
        ld      bc,#l__.bss
        ld      a,b
        or      c
        jr      z,.zxrom_no_gnu_bss
        ld      hl,#s__.bss
        ld      d,h
        ld      e,l
        inc     de
        ld      (hl),#0
        dec     bc
        ld      a,b
        or      c
        jr      z,.zxrom_no_gnu_bss
        ldir
.zxrom_no_gnu_bss:
        ld      de,#s__INITIALIZED
        ld      hl,#s__INITIALIZER
        ld      bc,#l__INITIALIZER
        ld      a,b
        or      c
        jr      z,.zxesxdos_no_init
        ldir
.zxesxdos_no_init:

        .area   _GSFINAL
        ret

        .area   _DATA

        .area   _INITIALIZED

        .area   _BSS

        .area   _INITIALIZER

        .area   _HEAP
