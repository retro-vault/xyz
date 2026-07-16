;--------------------------------------------------------
; File Created by SDCC : free open source ISO C Compiler
; Version 4.5.0 #15242 (Linux)
;--------------------------------------------------------
; Processed by Z88DK
;--------------------------------------------------------

	EXTERN __divschar
	EXTERN __divschar_callee
	EXTERN __divsint
	EXTERN __divsint_callee
	EXTERN __divslong
	EXTERN __divslong_callee
	EXTERN __divslonglong
	EXTERN __divslonglong_callee
	EXTERN __divsuchar
	EXTERN __divsuchar_callee
	EXTERN __divuchar
	EXTERN __divuchar_callee
	EXTERN __divuint
	EXTERN __divuint_callee
	EXTERN __divulong
	EXTERN __divulong_callee
	EXTERN __divulonglong
	EXTERN __divulonglong_callee
	EXTERN __divuschar
	EXTERN __divuschar_callee
	EXTERN __modschar
	EXTERN __modschar_callee
	EXTERN __modsint
	EXTERN __modsint_callee
	EXTERN __modslong
	EXTERN __modslong_callee
	EXTERN __modslonglong
	EXTERN __modslonglong_callee
	EXTERN __modsuchar
	EXTERN __modsuchar_callee
	EXTERN __moduchar
	EXTERN __moduchar_callee
	EXTERN __moduint
	EXTERN __moduint_callee
	EXTERN __modulong
	EXTERN __modulong_callee
	EXTERN __modulonglong
	EXTERN __modulonglong_callee
	EXTERN __moduschar
	EXTERN __moduschar_callee
	EXTERN __mulint
	EXTERN __mulint_callee
	EXTERN __mullong
	EXTERN __mullong_callee
	EXTERN __mullonglong
	EXTERN __mullonglong_callee
	EXTERN __mulschar
	EXTERN __mulschar_callee
	EXTERN __mulsuchar
	EXTERN __mulsuchar_callee
	EXTERN __muluchar
	EXTERN __muluchar_callee
	EXTERN __muluschar
	EXTERN __muluschar_callee
	EXTERN __rlslonglong
	EXTERN __rlslonglong_callee
	EXTERN __rlulonglong
	EXTERN __rlulonglong_callee
	EXTERN __rrslonglong
	EXTERN __rrslonglong_callee
	EXTERN __rrulonglong
	EXTERN __rrulonglong_callee
	EXTERN ___mulsint2slong
	EXTERN ___mulsint2slong_callee
	EXTERN ___muluint2ulong
	EXTERN ___muluint2ulong_callee
	EXTERN ___sdcc_call_hl
	EXTERN ___sdcc_call_iy
	EXTERN ___sdcc_enter_ix
	EXTERN banked_call
	EXTERN _banked_ret
	EXTERN ___fs2schar
	EXTERN ___fs2schar_callee
	EXTERN ___fs2sint
	EXTERN ___fs2sint_callee
	EXTERN ___fs2slong
	EXTERN ___fs2slong_callee
	EXTERN ___fs2slonglong
	EXTERN ___fs2slonglong_callee
	EXTERN ___fs2uchar
	EXTERN ___fs2uchar_callee
	EXTERN ___fs2uint
	EXTERN ___fs2uint_callee
	EXTERN ___fs2ulong
	EXTERN ___fs2ulong_callee
	EXTERN ___fs2ulonglong
	EXTERN ___fs2ulonglong_callee
	EXTERN ___fsadd
	EXTERN ___fsadd_callee
	EXTERN ___fsdiv
	EXTERN ___fsdiv_callee
	EXTERN ___fseq
	EXTERN ___fseq_callee
	EXTERN ___fsgt
	EXTERN ___fsgt_callee
	EXTERN ___fslt
	EXTERN ___fslt_callee
	EXTERN ___fsmul
	EXTERN ___fsmul_callee
	EXTERN ___fsneq
	EXTERN ___fsneq_callee
	EXTERN ___fssub
	EXTERN ___fssub_callee
	EXTERN ___schar2fs
	EXTERN ___schar2fs_callee
	EXTERN ___sint2fs
	EXTERN ___sint2fs_callee
	EXTERN ___slong2fs
	EXTERN ___slong2fs_callee
	EXTERN ___slonglong2fs
	EXTERN ___slonglong2fs_callee
	EXTERN ___uchar2fs
	EXTERN ___uchar2fs_callee
	EXTERN ___uint2fs
	EXTERN ___uint2fs_callee
	EXTERN ___ulong2fs
	EXTERN ___ulong2fs_callee
	EXTERN ___ulonglong2fs
	EXTERN ___ulonglong2fs_callee
	EXTERN ____sdcc_2_copy_src_mhl_dst_deix
	EXTERN ____sdcc_2_copy_src_mhl_dst_bcix
	EXTERN ____sdcc_4_copy_src_mhl_dst_deix
	EXTERN ____sdcc_4_copy_src_mhl_dst_bcix
	EXTERN ____sdcc_4_copy_src_mhl_dst_mbc
	EXTERN ____sdcc_4_ldi_nosave_bc
	EXTERN ____sdcc_4_ldi_save_bc
	EXTERN ____sdcc_4_push_hlix
	EXTERN ____sdcc_4_push_mhl
	EXTERN ____sdcc_lib_setmem_hl
	EXTERN ____sdcc_ll_add_de_bc_hl
	EXTERN ____sdcc_ll_add_de_bc_hlix
	EXTERN ____sdcc_ll_add_de_hlix_bc
	EXTERN ____sdcc_ll_add_de_hlix_bcix
	EXTERN ____sdcc_ll_add_deix_bc_hl
	EXTERN ____sdcc_ll_add_deix_hlix
	EXTERN ____sdcc_ll_add_hlix_bc_deix
	EXTERN ____sdcc_ll_add_hlix_deix_bc
	EXTERN ____sdcc_ll_add_hlix_deix_bcix
	EXTERN ____sdcc_ll_asr_hlix_a
	EXTERN ____sdcc_ll_asr_mbc_a
	EXTERN ____sdcc_ll_copy_src_de_dst_hlix
	EXTERN ____sdcc_ll_copy_src_de_dst_hlsp
	EXTERN ____sdcc_ll_copy_src_deix_dst_hl
	EXTERN ____sdcc_ll_copy_src_deix_dst_hlix
	EXTERN ____sdcc_ll_copy_src_deixm_dst_hlsp
	EXTERN ____sdcc_ll_copy_src_desp_dst_hlsp
	EXTERN ____sdcc_ll_copy_src_hl_dst_de
	EXTERN ____sdcc_ll_copy_src_hlsp_dst_de
	EXTERN ____sdcc_ll_copy_src_hlsp_dst_deixm
	EXTERN ____sdcc_ll_lsl_hlix_a
	EXTERN ____sdcc_ll_lsl_mbc_a
	EXTERN ____sdcc_ll_lsr_hlix_a
	EXTERN ____sdcc_ll_lsr_mbc_a
	EXTERN ____sdcc_ll_push_hlix
	EXTERN ____sdcc_ll_push_mhl
	EXTERN ____sdcc_ll_sub_de_bc_hl
	EXTERN ____sdcc_ll_sub_de_bc_hlix
	EXTERN ____sdcc_ll_sub_de_hlix_bc
	EXTERN ____sdcc_ll_sub_de_hlix_bcix
	EXTERN ____sdcc_ll_sub_deix_bc_hl
	EXTERN ____sdcc_ll_sub_deix_hlix
	EXTERN ____sdcc_ll_sub_hlix_bc_deix
	EXTERN ____sdcc_ll_sub_hlix_deix_bc
	EXTERN ____sdcc_ll_sub_hlix_deix_bcix
	EXTERN ____sdcc_load_debc_deix
	EXTERN ____sdcc_load_dehl_deix
	EXTERN ____sdcc_load_debc_mhl
	EXTERN ____sdcc_load_hlde_mhl
	EXTERN ____sdcc_store_dehl_bcix
	EXTERN ____sdcc_store_debc_hlix
	EXTERN ____sdcc_store_debc_mhl
	EXTERN ____sdcc_cpu_pop_ei
	EXTERN ____sdcc_cpu_pop_ei_jp
	EXTERN ____sdcc_cpu_push_di
	EXTERN ____sdcc_outi
	EXTERN ____sdcc_outi_128
	EXTERN ____sdcc_outi_256
	EXTERN ____sdcc_ldi
	EXTERN ____sdcc_ldi_128
	EXTERN ____sdcc_ldi_256
	EXTERN ____sdcc_4_copy_srcd_hlix_dst_deix
	EXTERN ____sdcc_4_and_src_mbc_mhl_dst_deix
	EXTERN ____sdcc_4_or_src_mbc_mhl_dst_deix
	EXTERN ____sdcc_4_xor_src_mbc_mhl_dst_deix
	EXTERN ____sdcc_4_or_src_dehl_dst_bcix
	EXTERN ____sdcc_4_xor_src_dehl_dst_bcix
	EXTERN ____sdcc_4_and_src_dehl_dst_bcix
	EXTERN ____sdcc_4_xor_src_mbc_mhl_dst_debc
	EXTERN ____sdcc_4_or_src_mbc_mhl_dst_debc
	EXTERN ____sdcc_4_and_src_mbc_mhl_dst_debc
	EXTERN ____sdcc_4_cpl_src_mhl_dst_debc
	EXTERN ____sdcc_4_xor_src_debc_mhl_dst_debc
	EXTERN ____sdcc_4_or_src_debc_mhl_dst_debc
	EXTERN ____sdcc_4_and_src_debc_mhl_dst_debc
	EXTERN ____sdcc_4_and_src_debc_hlix_dst_debc
	EXTERN ____sdcc_4_or_src_debc_hlix_dst_debc
	EXTERN ____sdcc_4_xor_src_debc_hlix_dst_debc

;--------------------------------------------------------
; Public variables in this module
;--------------------------------------------------------
	GLOBAL _main
	GLOBAL _suite_matrix
;--------------------------------------------------------
; Externals used
;--------------------------------------------------------
	GLOBAL _suite_add_test_real
	GLOBAL _suite_add_fixture
	GLOBAL _suite_setup
	GLOBAL _suite_run
	GLOBAL _Assert_real
	GLOBAL _unbcd
	GLOBAL _wcmatch
	GLOBAL _extract_bits_callee
	GLOBAL _extract_bits
	GLOBAL _msleep_fastcall
	GLOBAL _msleep
	GLOBAL _sleep_fastcall
	GLOBAL _sleep
	GLOBAL _t_delay
	GLOBAL _swapendian
	GLOBAL _outp_callee
	GLOBAL _outp
	GLOBAL _inp_fastcall
	GLOBAL _inp
	GLOBAL _isqrt_fastcall
	GLOBAL _isqrt
	GLOBAL _labs_fastcall
	GLOBAL _labs
	GLOBAL _abs_fastcall
	GLOBAL _abs
	GLOBAL __ldivu__callee
	GLOBAL __ldivu_
	GLOBAL __ldiv__callee
	GLOBAL __ldiv_
	GLOBAL __divu__callee
	GLOBAL __divu_
	GLOBAL __div__callee
	GLOBAL __div_
	GLOBAL _qsort_sdcc_callee
	GLOBAL _qsort_sdcc
	GLOBAL _qsort_sccz80_callee
	GLOBAL _qsort_sccz80
	GLOBAL _l_qsort_callee
	GLOBAL _l_qsort
	GLOBAL _l_bsearch_callee
	GLOBAL _l_bsearch
	GLOBAL _getopt
	GLOBAL _unsetenv
	GLOBAL _setenv
	GLOBAL _getenv_r
	GLOBAL _getenv
	GLOBAL _atexit_fastcall
	GLOBAL _exit_fastcall
	GLOBAL _atexit
	GLOBAL _exit
	GLOBAL _HeapInfo_callee
	GLOBAL _HeapInfo
	GLOBAL _HeapRealloc_callee
	GLOBAL _HeapRealloc
	GLOBAL _HeapAlloc_callee
	GLOBAL _HeapAlloc
	GLOBAL _HeapFree_callee
	GLOBAL _HeapFree
	GLOBAL _HeapCalloc_callee
	GLOBAL _HeapCalloc
	GLOBAL _HeapSbrk_callee
	GLOBAL _HeapSbrk
	GLOBAL _HeapCreate
	GLOBAL _mallinfo_callee
	GLOBAL _realloc_callee
	GLOBAL _calloc_callee
	GLOBAL _sbrk_callee
	GLOBAL _free_fastcall
	GLOBAL _malloc_fastcall
	GLOBAL _mallinfo
	GLOBAL _realloc
	GLOBAL _malloc
	GLOBAL _free
	GLOBAL _calloc
	GLOBAL _sbrk
	GLOBAL _mallinit
	GLOBAL _srand_fastcall
	GLOBAL _srand
	GLOBAL _rand
	GLOBAL _ulltoa_callee
	GLOBAL _ulltoa
	GLOBAL _strtoull_callee
	GLOBAL _strtoull
	GLOBAL _strtoll_callee
	GLOBAL _strtoll
	GLOBAL _lltoa_callee
	GLOBAL _lltoa
	GLOBAL _atoll_callee
	GLOBAL _atoll
	GLOBAL _utoa_callee
	GLOBAL _utoa
	GLOBAL _ultoa_callee
	GLOBAL _ultoa
	GLOBAL _strtoul_callee
	GLOBAL _strtoul
	GLOBAL _strtol_callee
	GLOBAL _strtol
	GLOBAL _ltoa_callee
	GLOBAL _ltoa
	GLOBAL _itoa_callee
	GLOBAL _itoa
	GLOBAL _atol_fastcall
	GLOBAL _atol
	GLOBAL _atoi_fastcall
	GLOBAL _atoi
	GLOBAL _optreset
	GLOBAL _optopt
	GLOBAL _optind
	GLOBAL _opterr
	GLOBAL _optarg
;--------------------------------------------------------
; special function registers
;--------------------------------------------------------
;--------------------------------------------------------
; ram data
;--------------------------------------------------------
	SECTION bss_compiler
_gridA:
	DEFS 3200
_gridB:
	DEFS 3200
;--------------------------------------------------------
; ram data
;--------------------------------------------------------

IF 0

; .area _INITIALIZED removed by z88dk


ENDIF

;--------------------------------------------------------
; absolute external ram data
;--------------------------------------------------------
	SECTION IGNORE
;--------------------------------------------------------
; global & static initialisations
;--------------------------------------------------------
	SECTION code_crt_init
;--------------------------------------------------------
; Home
;--------------------------------------------------------
	SECTION code_home
;--------------------------------------------------------
; code
;--------------------------------------------------------
	SECTION code_compiler
;	---------------------------------
; Function stencil
; ---------------------------------
_stencil:
	push	ix
	ld	ix,0
	add	ix,sp
	ld	hl, -12
	add	hl, sp
	ld	sp, hl
	ld	bc,0x0001
l_stencil_00105:
	ld	hl,0x0001
	ex	(sp), hl
	ld	l, c
	ld	h, b
	add	hl, hl
	add	hl, hl
	add	hl, bc
	add	hl, hl
	add	hl, hl
	add	hl, hl
	ld	(ix-10),l
	ld	(ix-9),h
	ld	e, c
	ld	d, b
	dec	de
	ld	l, e
	ld	h, d
	add	hl, hl
	add	hl, hl
	add	hl, de
	add	hl, hl
	add	hl, hl
	add	hl, hl
	ld	(ix-8),l
	ld	(ix-7),h
	inc	bc
	ld	l, c
	ld	h, b
	add	hl, hl
	add	hl, hl
	add	hl, bc
	add	hl, hl
	add	hl, hl
	add	hl, hl
	ld	(ix-6),l
	ld	(ix-5),h
l_stencil_00103:
	ld	a,(ix-10)
	add	a,(ix-12)
	ld	l, a
	ld	a,(ix-9)
	adc	a,(ix-11)
	ld	h, a
	add	hl, hl
	ld	(ix-4),l
	ld	(ix-3),h
	ld	a,(ix+4)
	add	a,(ix-4)
	ld	l, a
	ld	a,(ix+5)
	adc	a,(ix-3)
	ld	h, a
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ld	a,(ix-8)
	add	a,(ix-12)
	ld	l, a
	ld	a,(ix-7)
	adc	a,(ix-11)
	ld	h, a
	add	hl, hl
	ld	(ix-2),l
	ld	(ix-1),h
	ld	a,(ix-2)
	add	a,(ix+4)
	ld	l, a
	ld	a,(ix-1)
	adc	a,(ix+5)
	ld	h, a
	ld	a, (hl)
	inc	hl
	ld	h, (hl)
	ld	l, a
	add	hl, de
	push	hl
	pop	iy
	ld	a,(ix-6)
	add	a,(ix-12)
	ld	e, a
	ld	a,(ix-5)
	adc	a,(ix-11)
	ld	d, a
	ex	de, hl
	add	hl, hl
	ex	de, hl
	ld	l,(ix+4)
	ld	h,(ix+5)
	add	hl, de
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	add	iy, de
	pop	de
	push	de
	dec	de
	ld	l,(ix-10)
	ld	h,(ix-9)
	add	hl, de
	add	hl, hl
	ex	de,hl
	ld	l,(ix+4)
	ld	h,(ix+5)
	add	hl, de
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	add	iy, de
	ld	a,(ix-12)
	add	a,0x01
	ld	(ix-2),a
	ld	a,0x00
	adc	a,0x00
	ld	(ix-1),a
	ld	a,(ix-10)
	add	a,(ix-2)
	ld	e, a
	ld	a,(ix-9)
	adc	a,(ix-1)
	ld	d, a
	ex	de, hl
	add	hl, hl
	ex	de, hl
	ld	l,(ix+4)
	ld	h,(ix+5)
	add	hl, de
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	push	iy
	pop	hl
	add	hl, de
	ld	a,(ix-4)
	add	a,(ix+6)
	ld	e, a
	ld	a,(ix-3)
	adc	a,(ix+7)
	ld	d, a
	ld	a, l
	ld	(de), a
	inc	de
	ld	a, h
	ld	(de), a
	ld	a,(ix-2)
	ld	(ix-12),a
	ld	(ix-11),0x00
	ld	a,(ix-12)
	sub	a,0x27
	jp	C, l_stencil_00103
	ld	a, c
	sub	a,0x27
	jp	C, l_stencil_00105
	ld	sp, ix
	pop	ix
	ret
;	---------------------------------
; Function matrix_compute
; ---------------------------------
_matrix_compute:
	push	ix
	ld	ix,0
	add	ix,sp
	ld	hl, -8
	add	hl, sp
	ld	sp, hl
	xor	a, a
	ld	(ix-2),a
	ld	(ix-1),a
	ld	de,0xa5a5
	ld	(ix-8),+((_gridA) & 0xFF)
	ld	(ix-7),+((_gridA) / 256)
	ld	iy,_gridB
	ld	bc,0x0000
l_matrix_compute_00104:
	ld	l, e
	ld	h, d
	add	hl, hl
	add	hl, de
	add	hl, hl
	add	hl, hl
	add	hl, hl
	add	hl, hl
	add	hl, de
	add	hl, hl
	add	hl, hl
	add	hl, hl
	add	hl, de
	add	hl, hl
	add	hl, hl
	add	hl, de
	add	hl, hl
	add	hl, hl
	add	hl, de
	add	hl, hl
	add	hl, hl
	add	hl, de
	ex	de, hl
	ld	hl,0x3619
	add	hl, de
	ex	de, hl
	ld	l, c
	ld	h, b
	add	hl, hl
	ld	a, +((_gridA) & 0xFF)
	add	a, l
	ld	(ix-6),a
	ld	a, +((_gridA) / 256)
	adc	a, h
	ld	(ix-5),a
	ld	(ix-4),e
	ld	(ix-3),0x00
	push	hl
	ld	l,(ix-6)
	ld	h,(ix-5)
	ld	a,(ix-4)
	ld	(hl), a
	inc	hl
	ld	(hl),0x00
	pop	hl
	ld	a, +((_gridB) & 0xFF)
	add	a, l
	ld	l, a
	ld	a, +((_gridB) / 256)
	adc	a, h
	ld	h, a
	ld	a,(ix-4)
	ld	(hl), a
	inc	hl
	ld	(hl),0x00
	inc	bc
	ld	a, c
	sub	a,0x40
	ld	a, b
	rla
	ccf
	rra
	sbc	a,0x86
	jr	C,l_matrix_compute_00104
	ld	c,0x00
l_matrix_compute_00106:
	push	bc
	push	iy
	push	iy
	ld	l,(ix-8)
	ld	h,(ix-7)
	push	hl
	call	_stencil
	pop	af
	pop	af
	pop	iy
	pop	bc
	pop	de
	push	de
	inc	sp
	inc	sp
	push	iy
	push	de
	pop	iy
	inc	c
	ld	a, c
	sub	a,0x10
	jr	C,l_matrix_compute_00106
	ld	bc,0x0000
l_matrix_compute_00108:
	ld	l, c
	ld	h, b
	add	hl, hl
	pop	de
	push	de
	add	hl, de
	ld	a, (hl)
	inc	hl
	ld	e, (hl)
	add	a,(ix-2)
	ld	(ix-2),a
	ld	a, e
	adc	a,(ix-1)
	ld	(ix-1),a
	inc	bc
	ld	a, c
	sub	a,0x40
	ld	a, b
	rla
	ccf
	rra
	sbc	a,0x86
	jr	C,l_matrix_compute_00108
	ld	l,(ix-2)
	ld	h,(ix-1)
	ld	sp, ix
	pop	ix
	ret
;	---------------------------------
; Function matrix_run
; ---------------------------------
_matrix_run:
	call	_matrix_compute
	ld	de,___str_1+0
	ld	bc,___str_0+0
	ld	a, l
	sub	a,0x20
	jr	NZ,l_matrix_run_00103
	ld	a, h
	sub	a,0x8d
	jr	NZ,l_matrix_run_00103
	ld	a,0x01
	jr	l_matrix_run_00104
l_matrix_run_00103:
	xor	a,a
l_matrix_run_00104:
	ld	h,0x00
	push	de
	ld	de,0x0049
	push	de
	push	bc
	ld	l, a
	push	hl
	call	_Assert_real
	pop	af
	pop	af
	pop	af
	pop	af
	ret
	SECTION rodata_compiler
___str_0:
	DEFM "/home/tstih/data/retro-vault/xyz/x/tests/benchmarks/z88dk24/"
	DEFM "upstream/matrixbench/matrixbench.c"
	DEFB 0x00
	SECTION rodata_compiler
___str_1:
	DEFM "2D stencil sweep checksum (host-verified)"
	DEFB 0x00
	SECTION code_compiler
;	---------------------------------
; Function suite_matrix
; ---------------------------------
_suite_matrix:
	ld	hl,___str_2
	push	hl
	call	_suite_setup
	ld	hl,_matrix_run
	ex	(sp),hl
	ld	hl,___str_3
	push	hl
	call	_suite_add_test_real
	pop	af
	pop	af
	jp	_suite_run
	SECTION rodata_compiler
___str_2:
	DEFM "2D Stencil Tests"
	DEFB 0x00
	SECTION rodata_compiler
___str_3:
	DEFM "matrix_run"
	DEFB 0x00
	SECTION code_compiler
;	---------------------------------
; Function main
; ---------------------------------
_main:
	call	_suite_matrix
	jp	_exit_fastcall
	SECTION IGNORE
