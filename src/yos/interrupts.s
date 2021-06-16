        ;; interrupts.s
        ;;
        ;; zx spectrum interrupts
        ;;
        ;; note:
        ;;      _intr_set_vect is NOT guarded by ei/di!
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2021 Tomaz Stih
        ;;
		;; 2021-06-16   tstih
		.module interrupts

		.globl	_ir_enable
		.globl	_ir_disable
	
		.area	_CODE


		;; -------------------------
		;; extern void ir_disable();
		;; -------------------------
        ;; executes di with ref count.
        ;; affects: hl
_ir_disable::		
		di
		ld		hl,#ir_refcount
		inc		(hl)
		ret


		;; ------------------------
		;; extern void ir_enable();
		;; ------------------------
        ;; executes ei with ref count.
        ;; affects: af
_ir_enable::		
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
		ret
	

		.area	_INITIALIZED
ir_refcount:
		.ds		1


		.area	_INITIALIZER
init_ir_refcount:
		.byte	0
