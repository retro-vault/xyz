        ;; crt0.s  (sys backend: emu)
        ;;
        ;; Starts a flat emulator binary, initializes data/BSS, calls main(),
        ;; then reports the result through _exit.

        .module crt0
        .optsdcc -mz80 sdcccall(1)

        .globl  _main
        .globl  _exit
        .globl  _entry
        .globl  s__BSS
        .globl  l__BSS
        .globl  s__INITIALIZED
        .globl  s__INITIALIZER
        .globl  l__INITIALIZER

        .equ    STACK_TOP,0xfa00

        .area   _CODE
_entry::
        ld      sp,#STACK_TOP
        call    .gsinit
        call    _main
        ex      de,hl
        call    _exit
.crt0_halt:
        halt
        jr      .crt0_halt

        .area   _GSINIT
.gsinit:
        ld      bc,#l__BSS
        ld      a,b
        or      a,c
        jr      z,.gsinit_no_bss
        ld      hl,#s__BSS
        ld      d,h
        ld      e,l
        inc     de
        ld      (hl),#0
        dec     bc
        ld      a,b
        or      a,c
        jr      z,.gsinit_no_bss
        ldir
.gsinit_no_bss:
        ld      de,#s__INITIALIZED
        ld      hl,#s__INITIALIZER
        ld      bc,#l__INITIALIZER
        ld      a,b
        or      a,c
        jr      z,.gsinit_no_init
        ldir
.gsinit_no_init:
        .area   _GSFINAL
        ret

        .area   _DATA
        .area   _INITIALIZED
        .area   _BSS
        .area   _INITIALIZER
        .area   _HEAP
