        ;; crt0.s -- ZX Spectrum 48K replacement ROM startup
        ;;
        ;; ROM is 0x0000..0x3FFF. The display occupies 0x4000..0x5AFF;
        ;; writable C state starts at 0x5B00. xld packs the relocated _DATA
        ;; bytes into ROM and defines s__DATA_LOAD for this reset copy.

        .module crt0
        .optsdcc -mz80 sdcccall(1)

        .globl  _main
        .globl  __exit
        .globl  _entry
        .globl  _zx_console_init
        .globl  _zx_console_putc_a
        .globl  s__DATA
        .globl  l__DATA
        .globl  s__DATA_LOAD
        .globl  s__BSS
        .globl  l__BSS
        .globl  s__INITIALIZED
        .globl  s__INITIALIZER
        .globl  l__INITIALIZER

ZX_STACK_TOP   .equ    0xffff

        .area   _HEADER
        jp      _entry
        .ds     5
        jp      _entry
        .ds     5
        jp      .zxrom_rst10
        .ds     5
        jp      _entry
        .ds     5
        jp      _entry
        .ds     5
        jp      _entry
        .ds     5
        jp      _entry
        .ds     5
        jp      .zxrom_interrupt
        .ds     5
        .ds     38
.zxrom_nmi:
        retn
.zxrom_rst10:
        jp      _zx_console_putc_a
.zxrom_interrupt:
        ei
        reti

        .area   _CODE
_entry::
        di
        ld      sp,#ZX_STACK_TOP
        call    .zxrom_init
        call    _zx_console_init
        call    _main
        ex      de,hl
        jp      __exit

        .area   _GSINIT
.zxrom_init:
        ld      hl,#s__DATA_LOAD
        ld      de,#s__DATA
        ld      bc,#l__DATA
        ld      a,b
        or      c
        jr      z,.zxrom_no_data
        ldir
.zxrom_no_data:
        ld      de,#s__INITIALIZED
        ld      hl,#s__INITIALIZER
        ld      bc,#l__INITIALIZER
        ld      a,b
        or      c
        jr      z,.zxrom_no_init
        ldir
.zxrom_no_init:
        ld      bc,#l__BSS
        ld      a,b
        or      c
        jr      z,.zxrom_no_bss
        ld      hl,#s__BSS
        ld      d,h
        ld      e,l
        inc     de
        ld      (hl),#0
        dec     bc
        ld      a,b
        or      c
        jr      z,.zxrom_no_bss
        ldir
.zxrom_no_bss:
        .area   _GSFINAL
        ret

        .area   _CONST
        .area   _INITIALIZER
        .area   _DATA
        .area   _INITIALIZED
        .area   _BSS
        .area   _HEAP
