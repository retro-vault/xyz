        ;; crt0.s
        ;; zx spectrum ram startup code
        ;;
        ;; tomaz stih, apr 9 2013
        .module crt0

        .globl  _sys_vec_tbl
        .globl  _sys_stack
        .globl  _sys_heap
        .globl  _sys_tarpit
        .globl  ___sdcc_call_hl

        .area   _HEADER (ABS)
        di                              ; disable interrupts
init:
        ld      sp,#sysstack            ; now sp to OS stack (on bss)
        call    gsinit                  ; init static vars

        ;; start the os
        call    _main

        ;; install IM2 handler
        ld      a,#0x39                 ; im2 trick
        ld      i,a
        ld      hl,#0xffff
        ld      (hl),#0x18              ; jr upcode

        ;; start the scheduler
        im      2                       ; im 2, 50Hz interrupt on ZX Spectrum
        ei                              ; enable interrupts

_sys_tarpit::
        halt                            ; halt
        jr      _sys_tarpit             ; loop to eternity

        ;; SDCC glue.
___sdcc_call_hl::
	jp	(hl)

        ;; TODO: rewire temporary handlers
syscall:
        ret

        ;; (linker documentation) where specific ordering is desired -
        ;; the first linker input  file should  have  the area definitions
        ;; in the desired order
        .area   _HOME
        .area   _CODE
        .area   _GSINIT
        .area   _GSFINAL
        .area   _DATA
        .area   _BSS
        .area   _HEAP
        .area   _IM2

        ;; this area contains data initialization code -
        ;; unlike gnu toolchain which generates data, sdcc generates
        ;; initialization code for every initialized global
        ;; variable. and it puts this code into _GSINIT area
        .area   _GSINIT
gsinit:
        .area   _GSFINAL
        ret

        .area   _BSS
        ;; 512 bytes of operating system stack
        .ds     512
_sys_stack::
        .area   _HEAP
_sys_heap::
        .area   _IM2
        ;; vector jump table at 0xfff0
_sys_vec_tbl::
        jp      syscall                 ; fff0
        .db     0
        jp      _scheduler              ; fff4
        .db 0