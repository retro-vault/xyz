        ;; crt0.s  (sys backend: none — bare-metal template)
        ;;
        ;; Program entry point.  When you copy this backend to a new target:
        ;;   1. Set STACK_TOP to the top of usable RAM (the stack grows down).
        ;;   2. Everything else (zero .bss, copy the initialised-data image,
        ;;      call main(), route the result through exit()) is generic.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

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

        ;; TODO: top of usable RAM on your target.
STACK_TOP       .equ 0xffff

        .area   _CODE
_entry::
        ld      sp,#STACK_TOP           ; TODO: stack grows down from here
        call    .gsinit
        call    _main
        ex      de,hl                   ; HL = main() return value
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
        .area   _HEAP                   ; last: __heap_base marks top of image
