        ; crt0.s -- ZX Spectrum 48K esxDOS program startup
        ;
        ; Load at 0x8000 so BASIC can finish its loader before entry.
        ; After entry the program owns lower RAM too; it never returns
        ; to BASIC. IY retains the conventional ROM-call value.

        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module crt0
        .optsdcc -mz80 sdcccall(1)

        .globl  _main
        .globl  __exit
        .globl  _entry
        .globl  _zx_console_init
        .globl  s__BSS
        .globl  l__BSS
        .globl  s__INITIALIZED
        .globl  s__INITIALIZER
        .globl  l__INITIALIZER

ZX_STACK_TOP    .equ    0xffff

        .area   _CODE

        ; _entry
        ; inputs: booted Sinclair ROM and esxDOS, entry at 0x8000.
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
        .area   _STACK
