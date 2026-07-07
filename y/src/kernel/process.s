	.module	xcc_output
	.optsdcc	-mz80 sdcccall(1)
	.area	_DATA
	.globl	_process_first
_process_first:
	.ds	2
	.globl	_process_last_error
_process_last_error:
	.ds	1
	.globl	__evt_first
	.globl	__tmr_first
	.globl	__svc_first
	.globl	_thread_current
	.area	_CODE
_find_owned:
	; O3 sdcc-style helper fast path: guarded owner search
	ld	c, #0
__find_owned_loop_0:
	ld	a, h
	or	a, l
	jr	z, __find_owned_end_3
	push	hl
	inc	hl
	inc	hl
	ld	a, (hl)
	cp	e
	jr	nz, __find_owned_next_1
	inc	hl
	ld	a, (hl)
	cp	d
	jr	z, __find_owned_found_2
__find_owned_next_1:
	pop	hl
	inc	c
	jr	z, __find_owned_end_3
	ld	a, (hl)
	inc	hl
	ld	h, (hl)
	ld	l, a
	jr	__find_owned_loop_0
__find_owned_found_2:
	pop	hl
	ex	de, hl
	ret
__find_owned_end_3:
	ld	de, #0
	ret
	.area	_CODE
	.globl	_process_start
_process_start:
	; sdcccall(1) prologue: process_start (locals=0, temp_frame=18, stack_params=2)
	.globl	__sdcc_enter_ix
	call	__sdcc_enter_ix
	ld	-2(ix), l
	ld	-1(ix), h
	ld	-4(ix), e
	ld	-3(ix), d
	ld	hl, #-18
	add	hl, sp
	ld	sp, hl
	; receive (sdcccall1) register param handled by prologue
	; receive (sdcccall1) register param handled by prologue
	; receive (sdcccall1) param stack_size at 4(ix)
	ld	hl, #_process_first
	ld	-6(ix), l
	ld	-5(ix), h
	ld	hl, #0
	push	hl
	ld	de, #15
	ld	l, -6(ix)
	ld	h, -5(ix)
	.globl	_so_create
	call	_so_create
	ld	h, d
	ld	l, e
	ld	-8(ix), l
	ld	-7(ix), h
	ld	a, h
	or	a, l
	jp	z, __xcc_L15
__xcc_L13:
	ld	l, -8(ix)
	ld	h, -7(ix)
	ld	de, #5
	add	hl, de
	ld	-10(ix), l
	ld	-9(ix), h
	ld	e, -2(ix)
	ld	d, -1(ix)
	.globl	_strcpy
	call	_strcpy
	ld	l, -8(ix)
	ld	h, -7(ix)
	ld	de, #4
	add	hl, de
	xor	a
	ld	(hl), a
	ld	l, -8(ix)
	ld	h, -7(ix)
	push	hl
	ld	e, 4(ix)
	ld	d, 5(ix)
	ld	l, -4(ix)
	ld	h, -3(ix)
	.globl	_thread_create
	call	_thread_create
	ex	de, hl
	ld	-12(ix), l
	ld	-11(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	ld	de, #13
	add	hl, de
	ld	e, -12(ix)
	ld	d, -11(ix)
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -12(ix)
	ld	h, -11(ix)
	ld	a, h
	or	a, l
	jr	nz, __xcc_L18
__xcc_L16:
	ld	hl, #_process_first
	ld	-14(ix), l
	ld	-13(ix), h
	ld	e, -8(ix)
	ld	d, -7(ix)
	.globl	_so_destroy
	call	_so_destroy
	ld	hl, #0
	ex	de, hl
	jr	__process_start_end
__xcc_L18:
	ld	l, -8(ix)
	ld	h, -7(ix)
	ld	de, #13
	add	hl, de
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ld	-16(ix), e
	ld	-15(ix), d
	ld	l, -16(ix)
	ld	h, -15(ix)
	ld	de, #22
	add	hl, de
	ld	e, -8(ix)
	ld	d, -7(ix)
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -8(ix)
	ld	h, -7(ix)
	ld	de, #13
	add	hl, de
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	.globl	_thread_resume
	call	_thread_resume
__xcc_L15:
	ld	l, -8(ix)
	ld	h, -7(ix)
	ex	de, hl
__process_start_end:
	; epilogue: process_start
	ld	sp, ix
	pop	ix
	pop	bc
	inc	sp
	inc	sp
	push	bc
	ret
	.area	_CODE
	.globl	_process_load
_process_load:
	; sdcccall(1) prologue: process_load (locals=18, temp_frame=40, stack_params=2)
	.globl	__sdcc_enter_ix
	call	__sdcc_enter_ix
	ld	-19(ix), a
	ld	-21(ix), e
	ld	-20(ix), d
	ld	hl, #-58
	add	hl, sp
	ld	sp, hl
	; receive (sdcccall1) register param handled by prologue
	; receive (sdcccall1) register param handled by prologue
	; receive (sdcccall1) param stack_size at 4(ix)
	ld	hl, #48
	add	hl, sp
	ld	b, h
	ld	c, l
	ld	d, b
	ld	e, c
	ld	l, -21(ix)
	ld	h, -20(ix)
	.globl	_mdr_make_name10
	call	_mdr_make_name10
	ld	hl, #40
	add	hl, sp
	ld	-23(ix), l
	ld	-22(ix), h
	xor	a
	ld	-24(ix), a
	ld	l, -23(ix)
	ld	h, -22(ix)
	ld	-26(ix), l
	ld	-25(ix), h
	ld	l, -21(ix)
	ld	h, -20(ix)
	ld	-28(ix), l
	ld	-27(ix), h
__xcc_inl___xcc_L0_0:
	ld	a, -24(ix)
	cp	#7
	jr	nc, __xcc_inl___xcc_L2_0
__xcc_inl___xcc_L3_0:
	ld	l, -28(ix)
	ld	h, -27(ix)
	ld	a, (hl)
	ld	-30(ix), a
	or	a, a
	jr	z, __xcc_inl___xcc_L2_0
__xcc_inl___xcc_L1_0:
	ld	l, -26(ix)
	ld	h, -25(ix)
	ld	a, -30(ix)
	ld	(hl), a
	inc	-24(ix)
	ld	l, -26(ix)
	ld	h, -25(ix)
	inc	hl
	ld	-26(ix), l
	ld	-25(ix), h
	ld	l, -28(ix)
	ld	h, -27(ix)
	inc	hl
	ld	-28(ix), l
	ld	-27(ix), h
	jr	__xcc_inl___xcc_L0_0
__xcc_inl___xcc_L2_0:
	ld	c, -24(ix)
	ld	b, #0
	ld	l, -23(ix)
	ld	h, -22(ix)
	add	hl, bc
	xor	a
	ld	(hl), a
	ld	(_process_last_error), a
	push	ix
	pop	hl
	ld	bc, #-10
	add	hl, bc
	ld	b, h
	ld	c, l
	ld	d, b
	ld	e, c
	ld	a, -19(ix)
	.globl	_mdr_find_file_size
	call	_mdr_find_file_size
	ex	de, hl
	ld	-32(ix), l
	ld	-31(ix), h
	ld	de, #12
	or	a, a
	sbc	hl, de
	jr	nc, __xcc_L21
__xcc_L19:
	ld	a, #1
	ld	(_process_last_error), a
	ld	hl, #0
	ex	de, hl
	jp	__process_load_end
__xcc_L21:
	ld	hl, #__heap
	ld	-34(ix), l
	ld	-33(ix), h
	ld	hl, #0
	push	hl
	ld	e, -32(ix)
	ld	d, -31(ix)
	ld	l, -34(ix)
	ld	h, -33(ix)
	.globl	_mem_allocate
	call	_mem_allocate
	ld	h, d
	ld	l, e
	ld	-36(ix), l
	ld	-35(ix), h
	ld	a, h
	or	a, l
	jr	nz, __xcc_L24
__xcc_L22:
	ld	a, #2
	ld	(_process_last_error), a
	ld	hl, #0
	ex	de, hl
	jp	__process_load_end
__xcc_L24:
	push	ix
	pop	hl
	ld	bc, #-10
	add	hl, bc
	ld	-38(ix), l
	ld	-37(ix), h
	ld	l, -36(ix)
	ld	h, -35(ix)
	push	hl
	ld	e, -38(ix)
	ld	d, -37(ix)
	ld	a, -19(ix)
	.globl	_mdr_load
	call	_mdr_load
	or	a, a
	jr	z, __xcc_L27
__xcc_L25:
	ld	a, #3
	ld	(_process_last_error), a
	ld	hl, #__heap
	ld	-40(ix), l
	ld	-39(ix), h
	ld	e, -36(ix)
	ld	d, -35(ix)
	.globl	_mem_free
	call	_mem_free
	ld	hl, #0
	ex	de, hl
	jp	__process_load_end
__xcc_L27:
	ld	l, -36(ix)
	ld	h, -35(ix)
	.globl	__process_relocate
	call	__process_relocate
	or	a, a
	jr	z, __xcc_L30
__xcc_L28:
	ld	a, #4
	ld	(_process_last_error), a
	ld	hl, #__heap
	ld	-42(ix), l
	ld	-41(ix), h
	ld	e, -36(ix)
	ld	d, -35(ix)
	.globl	_mem_free
	call	_mem_free
	ld	hl, #0
	ex	de, hl
	jp	__process_load_end
__xcc_L30:
	ld	l, -36(ix)
	ld	h, -35(ix)
	ld	de, #12
	add	hl, de
	ld	-44(ix), l
	ld	-43(ix), h
	ld	l, -36(ix)
	ld	h, -35(ix)
	ld	de, #8
	add	hl, de
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ld	-46(ix), e
	ld	-45(ix), d
	ld	l, -46(ix)
	ld	h, -45(ix)
	add	hl, hl
	add	hl, hl
	ld	b, h
	ld	c, l
	ld	l, -44(ix)
	ld	h, -43(ix)
	add	hl, bc
	ld	-48(ix), l
	ld	-47(ix), h
	ld	l, -36(ix)
	ld	h, -35(ix)
	ld	de, #4
	add	hl, de
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ld	-50(ix), e
	ld	-49(ix), d
	ld	l, -48(ix)
	ld	h, -47(ix)
	add	hl, de
	ld	-52(ix), l
	ld	-51(ix), h
	push	ix
	pop	hl
	ld	bc, #-18
	add	hl, bc
	ld	-54(ix), l
	ld	-53(ix), h
	ld	l, 4(ix)
	ld	h, 5(ix)
	ld	b, h
	ld	c, l
	push	hl
	ld	e, -52(ix)
	ld	d, -51(ix)
	ld	l, -54(ix)
	ld	h, -53(ix)
	.globl	_process_start
	call	_process_start
	ex	de, hl
	ld	-56(ix), l
	ld	-55(ix), h
	ld	a, h
	or	a, l
	jr	nz, __xcc_L33
__xcc_L31:
	ld	a, #5
	ld	(_process_last_error), a
	ld	hl, #__heap
	ld	-58(ix), l
	ld	-57(ix), h
	ld	e, -36(ix)
	ld	d, -35(ix)
	.globl	_mem_free
	call	_mem_free
	ld	hl, #0
	ex	de, hl
	jr	__process_load_end
__xcc_L33:
	ld	l, -36(ix)
	ld	h, -35(ix)
	ld	de, #5
	or	a, a
	sbc	hl, de
	ld	e, -56(ix)
	ld	d, -55(ix)
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -56(ix)
	ld	h, -55(ix)
	ex	de, hl
__process_load_end:
	; epilogue: process_load
	ld	sp, ix
	pop	ix
	pop	bc
	inc	sp
	inc	sp
	push	bc
	ret
	.area	_CODE
	.globl	_process_reap
_process_reap:
	; sdcccall(1) prologue: process_reap (locals=0, temp_frame=12, stack_params=0)
	.globl	__sdcc_enter_ix
	call	__sdcc_enter_ix
	; keep incoming register arg t161 live in register for first use
	ex	de, hl
	ld	hl, #-12
	add	hl, sp
	ld	sp, hl
	ex	de, hl
	; receive (sdcccall1) register param handled by prologue
	; materialize incoming arg temp t161 for later reuse
	ld	-2(ix), l
	ld	-1(ix), h
	ld	a, h
	or	a, l
	jr	nz, __xcc_L36
__xcc_L34:
	jp	__process_reap_end
__xcc_L36:
	.globl	_enter_critical_section
	call	_enter_critical_section
	ld	l, -2(ix)
	ld	h, -1(ix)
	.globl	_process_has_threads
	call	_process_has_threads
	or	a, a
	jr	z, __xcc_L39
__xcc_L37:
	.globl	_leave_critical_section
	call	_leave_critical_section
	jp	__process_reap_end
__xcc_L39:
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	de, #13
	add	hl, de
	ld	de, #0
	ld	(hl), e
	inc	hl
	ld	(hl), d
__xcc_L40:
	ld	hl, (#__evt_first)
	ld	-4(ix), l
	ld	-3(ix), h
	ld	e, -2(ix)
	ld	d, -1(ix)
	.globl	_find_owned
	call	_find_owned
	ld	a, d
	or	a, e
	jr	z, __xcc_L43
__xcc_L41:
	ex	de, hl
	.globl	_evt_destroy
	call	_evt_destroy
	jr	__xcc_L40
__xcc_L43:
	ld	hl, (#__tmr_first)
	ld	-6(ix), l
	ld	-5(ix), h
	ld	e, -2(ix)
	ld	d, -1(ix)
	.globl	_find_owned
	call	_find_owned
	ld	a, d
	or	a, e
	jr	z, __xcc_L46
__xcc_L44:
	ex	de, hl
	.globl	_tmr_uninstall
	call	_tmr_uninstall
	jr	__xcc_L43
__xcc_L46:
	ld	hl, (#__svc_first)
	ld	-8(ix), l
	ld	-7(ix), h
	ld	e, -2(ix)
	ld	d, -1(ix)
	.globl	_find_owned
	call	_find_owned
	ld	a, d
	or	a, e
	jr	z, __xcc_L48
__xcc_L47:
	ex	de, hl
	.globl	_svc_unregister
	call	_svc_unregister
	jr	__xcc_L46
__xcc_L48:
	ld	hl, #__heap
	ld	-10(ix), l
	ld	-9(ix), h
	ld	e, -2(ix)
	ld	d, -1(ix)
	.globl	_mem_free_owner
	call	_mem_free_owner
	ld	hl, #_process_first
	ld	-12(ix), l
	ld	-11(ix), h
	ld	e, -2(ix)
	ld	d, -1(ix)
	.globl	_so_destroy
	call	_so_destroy
	.globl	_leave_critical_section
	call	_leave_critical_section
__process_reap_end:
	; epilogue: process_reap
	.globl	__sdcc_leave_ix
	jp	__sdcc_leave_ix
	.area	_CODE
	.globl	_process_exit
_process_exit:
	; O3 sdcc-style helper fast path: current-thread exit wrapper
	ld	hl, (#_thread_current)
	ld	a, h
	or	a, l
	ret	z
	.globl	_thread_exit
	jp	_thread_exit
