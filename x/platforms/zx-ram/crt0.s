        ;; crt0.s -- ZX Spectrum 48K RAM program startup
        ;;
        ;; 0x5CCB is the first byte after the documented 48K ROM system
        ;; variables. It is also the normal default BASIC program area.

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

ZX_STACK_TOP   .equ    0xffff

        .area   _CODE
_entry::
        di
        ld      sp,#ZX_STACK_TOP
        call    .zxram_init
        call    _zx_console_init
        call    _main
        ex      de,hl
        jp      __exit

        .area   _GSINIT
.zxram_init:
        ld      bc,#l__BSS
        ld      a,b
        or      c
        jr      z,.zxram_no_bss
        ld      hl,#s__BSS
        ld      d,h
        ld      e,l
        inc     de
        ld      (hl),#0
        dec     bc
        ld      a,b
        or      c
        jr      z,.zxram_no_bss
        ldir
.zxram_no_bss:
        ld      de,#s__INITIALIZED
        ld      hl,#s__INITIALIZER
        ld      bc,#l__INITIALIZER
        ld      a,b
        or      c
        jr      z,.zxram_no_init
        ldir
.zxram_no_init:
        .area   _GSFINAL
        ret

        .area   _DATA
        .area   _INITIALIZED
        .area   _BSS
        .area   _INITIALIZER
        .area   _HEAP
