        ;; crt0rom.s
        ;; 
        ;; zx spectrum 48K rom startup code
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2021 Tomaz Stih
        ;;
        ;; 2021-06-16   tstih
        .module crt0rom

        .globl	_ir_enable
        .globl	_ir_disable
        .globl  __sys_vec_tbl
        .globl  _sys_vec_set
        .globl  _sys_vec_get
        .globl  __sys_stack
        .globl  __sys_heap
        .globl  __heap

        .area   _HEADER (ABS)
        .org    0x0000
        di                              ; disable interrupts
        jp      init                    ; init
        .db     0,0,0,0

        ;; rst 0x08
        jp      rst8
rst8ret:
        reti
        .db     0,0,0

        ;; rst 0x10
        jp      rst10
rst10ret:
        reti
        .db     0,0,0

        ;; rst 0x18
        jp      rst18
rst18ret:
        reti
        .db     0,0,0

        ;; rst 0x20
        jp      rst20
rst20ret:
        reti
        .db     0,0,0

        ;; rst 0x28
        jp  rst28
rst28ret:
        reti
        .db     0,0,0

        ;; rst 0x30
        jp      rst30
rst30ret:
        reti
        .db     0,0,0

        ;; rst 0x38 - im 1
        jp      rst38
rst38ret:
        reti
        ;; 41 bytes of free space for cofiguration.
        .dw     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .db     0

        ;; nmi interrupt 0x66
        jp      nmi
nmiret:
        retn

init:
        ld      sp,#__sys_stack         ; now sp to OS stack (on bss)
        call    gsinit                  ; init static vars

        ;; start interrupts
        im      1                       ; im 1, 50Hz interrupt on ZX Spectrum
        ei                              ; enable interrupts

        ;; call the main!
        call    _main

tarpit:
        halt                            ; halt
        jr      tarpit


        ;; ------------------------------------------------------------
        ;; extern void sys_vec_set(void (*handler)(), uint8_t vec_num);
        ;; ------------------------------------------------------------
        ;; affects: bc, de, hl
_sys_vec_set::
        call    _ir_disable
        pop		hl                      ; ret address / ignore
        pop		bc                      ; bc = handler
        pop		de                      ; d = undefined, e = vector number
        ;; restore stack
        push	de
        push	bc
        push	hl
        ld		d,#0x00                 ; de = 16 bit vector number
        ld		hl,#__sys_vec_tbl       ; vector table start
        add		hl,de
        add		hl,de
        add		hl,de
        inc		hl						; hl = hl + 3 * de + 1
        ;; hl now points to vector address in RAM		
        ;; so set it to handler value in bc
        ld		(hl),c
        inc		hl
        ld		(hl),b
        call    _ir_enable
        ret


        ;; ------------------------------------------
        ;; extern void *sys_vec_get(uint8_t vec_num);
        ;; ------------------------------------------
        ;; affects: hl, de
_sys_vec_get::
        call    _ir_disable
        pop     de                      ; d = undefied, e = #vector
        ld      d,#0                    ; de = 16bit vector number
        ld      hl,#__sys_vec_tbl       ; vector table to hl
        add		hl,de
        add		hl,de
        add		hl,de
        inc		hl                      ; hl = hl + 3 * de + 1
        ld      e,(hl)                  ; vector into de
        inc     hl
        ld      d,(hl)
        ex      de,hl                   ; and into hl
        call    _ir_enable
        ret


        ;; -------------------------
        ;; extern void ir_disable();
        ;; -------------------------
        ;; executes di with ref count.
        ;; affects: -
_ir_disable::	
        di
        push    hl
        ld		hl,#ir_refcount
        inc		(hl)
        pop     hl
        ret


        ;; ------------------------
        ;; extern void ir_enable();
        ;; ------------------------
        ;; executes ei with ref count.
        ;; affects: -
_ir_enable::
        push    af
        ld		a,(#ir_refcount)
        or		a
        jr		z,do_ei					; if a==0 then just ei		
        dec		a						; if a<>0 then dec a
        ld		(#ir_refcount),a	    ; write back to counter
        or		a						; and check for ei
        jr		nz,dont_ei				; not yet...
do_ei:		
        ei
dont_ei:	
        pop     af
        ret


        ;; this are in ROM, but will be copied to an area in RAM!
start_vectors:
        jp      rst8ret
        jp      rst10ret
        jp      rst18ret
        jp      rst20ret
        jp      rst28ret
        jp      rst30ret
        jp      rst38ret
        jp      nmiret
end_vectors:
        ;; (linker documentation) where specific ordering is desired -
        ;; the first linker input  file should  have  the area definitions
        ;; in the desired order
        .area   _HEADER
        .area   _CODE
        .area   _INITIALIZER
        .area   _INITFINAL
        .area   _GSINIT
        .area   _GSFINAL
        .area   _DATA
        .area   _INITIALIZED
        .area   _BSS
        .area   _HEAP

        ;; this area contains data initialization code -
        ;; unlike gnu toolchain which generates data, sdcc generates
        ;; initialization code for initialized global
        ;; variables. and it puts this code into _GSINIT area
        .area   _GSINIT
gsinit:
        ;; move vector table to RAM
        ld      hl,#start_vectors
        ld      de,#__sys_vec_tbl
        ld      bc,#end_vectors - #start_vectors
        ldir

        ;; initialize vars from initializer
        ld      de, #s__INITIALIZED
        ld      hl, #s__INITIALIZER
        ld      bc, #l__INITIALIZER
        ld      a, b
        or      a, c
        jr      z, gsinit_none
        ldir
gsinit_none:
        .area   _GSFINAL
        ret

        .area   _DATA
        ;; vector jump table in ram
__sys_vec_tbl::
rst8:   .ds     3
rst10:  .ds     3
rst18:  .ds     3
rst20:  .ds     3
rst28:  .ds     3
rst30:  .ds     3
rst38:  .ds     3
nmi:    .ds     3


        .area   _BSS
        ;; 512 bytes of operating system stack
        .ds     512
__sys_stack::


        .area   _HEAP
__sys_heap::
        ;; 1KB of system heap
        .ds     1024
__heap::


        .area	_INITIALIZED
ir_refcount:
        .ds		1


        .area	_INITIALIZER
init_ir_refcount:
        .byte	0
