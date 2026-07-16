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
	GLOBAL _suite_hash
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
_keys:
	DEFS 800
_slot_key:
	DEFS 2048
_slot_used:
	DEFS 1024
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
; Function hash_key
; ---------------------------------
_hash_key:
	push	ix
	ld	ix,0
	add	ix,sp
	ld	de,0x1505
	ld	c,0x00
l_hash_key_00102:
	ld	l, e
	ld	h, d
	add	hl, hl
	add	hl, hl
	add	hl, hl
	add	hl, hl
	add	hl, hl
	add	hl, de
	ex	de, hl
	ld	l,(ix+4)
	ld	h,(ix+5)
	ld	b,0x00
	add	hl, bc
	ld	l, (hl)
	ld	h,0x00
	add	hl, de
	ex	de, hl
	inc	c
	ld	a, c
	sub	a,0x04
	jr	C,l_hash_key_00102
	ex	de, hl
	pop	ix
	ret
;	---------------------------------
; Function key_eq
; ---------------------------------
_key_eq:
	ld	c,0x00
l_key_eq_00104:
	pop	de
	pop	hl
	push	hl
	push	de
	ld	b,0x00
	add	hl, bc
	ld	b, (hl)
	ld	iy,4
	add	iy, sp
	ld	a,(iy+0)
	add	a, c
	ld	e, a
	ld	a,(iy+1)
	adc	a,0x00
	ld	d, a
	ld	a, (de)
	sub	a, b
	jr	Z,l_key_eq_00105
	ld	hl,0x0000
	jr	l_key_eq_00106
l_key_eq_00105:
	inc	c
	ld	a, c
	sub	a,0x04
	jr	C,l_key_eq_00104
	ld	hl,0x0001
l_key_eq_00106:
	ret
;	---------------------------------
; Function ht_insert
; ---------------------------------
_ht_insert:
	push	ix
	ld	ix,0
	add	ix,sp
	ld	hl, -11
	add	hl, sp
	ld	sp, hl
	ld	l,(ix+4)
	ld	h,(ix+5)
	push	hl
	call	_hash_key
	pop	af
	ld	(ix-2),l
	ld	(ix-1),h
	ld	a,(ix-2)
	ld	(ix-6),a
	ld	a,(ix-1)
	and	a,0x01
	ld	(ix-5),a
l_ht_insert_00109:
	ld	a,(ix-6)
	ld	(ix-2),a
	ld	a,(ix-5)
	ld	(ix-1),a
	sla	(ix-2)
	rl	(ix-1)
	ld	a,(ix-2)
	add	a, +((_slot_used) & 0xFF)
	ld	(ix-11),a
	ld	a,(ix-1)
	adc	a, +((_slot_used) / 256)
	ld	(ix-10),a
	pop	hl
	push	hl
	ld	a, (hl)
	ld	(ix-4),a
	inc	hl
	ld	a, (hl)
	ld	(ix-3),a
	ld	a,(ix-6)
	ld	(ix-2),a
	ld	a,(ix-5)
	ld	(ix-1),a
	ld	b,0x02
l_ht_insert_00150:
	sla	(ix-2)
	rl	(ix-1)
	djnz	l_ht_insert_00150
	ld	a,(ix-6)
	ld	(ix-9),a
	ld	a,(ix-5)
	ld	(ix-8),a
	ld	a,(ix-3)
	or	a,(ix-4)
	jr	NZ,l_ht_insert_00103
	ld	a,(ix-2)
	add	a, +((_slot_key) & 0xFF)
	ld	(ix-7),a
	ld	a,(ix-1)
	adc	a, +((_slot_key) / 256)
	ld	(ix-6),a
	ld	(ix-1),0x00
l_ht_insert_00107:
	ld	a,(ix-7)
	add	a,(ix-1)
	ld	(ix-5),a
	ld	a,(ix-6)
	adc	a,0x00
	ld	(ix-4),a
	ld	a,(ix+4)
	add	a,(ix-1)
	ld	(ix-3),a
	ld	a,(ix+5)
	adc	a,0x00
	ld	(ix-2),a
	ld	l,(ix-3)
	ld	h,(ix-2)
	ld	a, (hl)
	ld	l,(ix-5)
	ld	h,(ix-4)
	ld	(hl), a
	inc	(ix-1)
	ld	a,(ix-1)
	sub	a,0x04
	jr	C,l_ht_insert_00107
	pop	hl
	push	hl
	ld	(hl),0x01
	inc	hl
	ld	(hl),0x00
	pop	de
	pop	hl
	push	hl
	push	de
	jr	l_ht_insert_00111
l_ht_insert_00103:
	ld	a, +((_slot_key) & 0xFF)
	add	a,(ix-2)
	ld	c, a
	ld	a, +((_slot_key) / 256)
	adc	a,(ix-1)
	ld	l,(ix+4)
	ld	h,(ix+5)
	push	hl
	ld	b, a
	push	bc
	call	_key_eq
	pop	af
	pop	af
	ld	a, h
	or	a, l
	jr	Z,l_ht_insert_00105
	pop	de
	pop	hl
	push	hl
	push	de
	jr	l_ht_insert_00111
l_ht_insert_00105:
	ld	c,(ix-6)
	ld	b,(ix-5)
	inc	bc
	ld	(ix-6),c
	ld	a, b
	and	a,0x01
	ld	(ix-5),a
	jp	l_ht_insert_00109
l_ht_insert_00111:
	ld	sp, ix
	pop	ix
	ret
;	---------------------------------
; Function ht_lookup
; ---------------------------------
_ht_lookup:
	push	ix
	ld	ix,0
	add	ix,sp
	ld	l,(ix+4)
	ld	h,(ix+5)
	push	hl
	call	_hash_key
	pop	af
	ld	c, l
	ld	a, h
	and	a,0x01
	ld	b, a
l_ht_lookup_00106:
	ld	l, c
	ld	h, b
	add	hl, hl
	ld	de,_slot_used
	add	hl, de
	ld	a, (hl)
	inc	hl
	ld	e, (hl)
	or	a, e
	jr	NZ,l_ht_lookup_00102
	ld	hl,0xffff
	jr	l_ht_lookup_00108
l_ht_lookup_00102:
	ld	l, c
	ld	h, b
	add	hl, hl
	add	hl, hl
	ld	de,_slot_key
	add	hl, de
	push	bc
	ld	e,(ix+4)
	ld	d,(ix+5)
	push	de
	push	hl
	call	_key_eq
	pop	af
	pop	af
	pop	bc
	ld	a, h
	or	a, l
	jr	Z,l_ht_lookup_00104
	ld	l, c
	ld	h, b
	jr	l_ht_lookup_00108
l_ht_lookup_00104:
	inc	bc
	ld	a, b
	and	a,0x01
	ld	b, a
	jr	l_ht_lookup_00106
l_ht_lookup_00108:
	pop	ix
	ret
;	---------------------------------
; Function hash_compute
; ---------------------------------
_hash_compute:
	push	ix
	ld	ix,0
	add	ix,sp
	ld	hl, -11
	add	hl, sp
	ld	sp, hl
	xor	a, a
	ld	(ix-5),a
	ld	(ix-4),a
	ld	hl,0xace1
	ex	(sp), hl
	xor	a, a
	ld	(ix-3),a
	ld	(ix-2),a
l_hash_compute_00123:
	ld	l,(ix-3)
	ld	h,(ix-2)
	add	hl, hl
	add	hl, hl
	ld	a, l
	add	a, +((_keys) & 0xFF)
	ld	(ix-9),a
	ld	a, h
	adc	a, +((_keys) / 256)
	ld	(ix-8),a
	ld	(ix-1),0x00
l_hash_compute_00109:
	pop	bc
	push	bc
	ld	l, c
	ld	h, b
	add	hl, hl
	add	hl, bc
	add	hl, hl
	add	hl, hl
	add	hl, hl
	add	hl, hl
	add	hl, bc
	add	hl, hl
	add	hl, hl
	add	hl, hl
	add	hl, bc
	add	hl, hl
	add	hl, hl
	add	hl, bc
	add	hl, hl
	add	hl, hl
	add	hl, bc
	add	hl, hl
	add	hl, hl
	add	hl, bc
	ld	(ix-7),l
	ld	(ix-6),h
	ld	a,(ix-7)
	add	a,0x19
	ld	(ix-11),a
	ld	a,(ix-6)
	adc	a,0x36
	ld	(ix-10),a
	ld	a,(ix-1)
	add	a,(ix-9)
	ld	c, a
	ld	a,0x00
	adc	a,(ix-8)
	ld	b, a
	push	bc
	ld	hl,0x0014
	push	hl
	ld	l,(ix-11)
	ld	h,(ix-10)
	push	hl
	call	__moduint_callee
	pop	bc
	ld	a, l
	add	a,0x61
	ld	(bc), a
	inc	(ix-1)
	ld	a,(ix-1)
	sub	a,0x04
	jr	C,l_hash_compute_00109
	inc	(ix-3)
	jr	NZ,l_hash_compute_00220
	inc	(ix-2)
l_hash_compute_00220:
	ld	a,(ix-3)
	sub	a,0xc8
	ld	a,(ix-2)
	rla
	ccf
	rra
	sbc	a,0x80
	jp	C, l_hash_compute_00123
	ld	(ix-1),0x00
l_hash_compute_00127:
	ld	de,0x0000
l_hash_compute_00113:
	ld	l, e
	ld	h, d
	add	hl, hl
	ld	bc,_slot_used
	add	hl, bc
	xor	a, a
	ld	(hl), a
	inc	hl
	ld	(hl), a
	inc	de
	ld	a, d
	xor	a,0x80
	sub	a,0x82
	jr	C,l_hash_compute_00113
	ld	bc,0x0000
l_hash_compute_00115:
	ld	l, c
	ld	h, b
	add	hl, hl
	add	hl, hl
	ld	de,_keys
	add	hl, de
	push	bc
	push	hl
	call	_ht_insert
	pop	af
	pop	bc
	ld	a, l
	add	a,(ix-5)
	ld	(ix-5),a
	ld	a, h
	adc	a,(ix-4)
	ld	(ix-4),a
	inc	bc
	ld	a, c
	sub	a,0xc8
	ld	a, b
	rla
	ccf
	rra
	sbc	a,0x80
	jr	C,l_hash_compute_00115
	ld	bc,0x0000
l_hash_compute_00117:
	ld	l, c
	ld	h, b
	add	hl, hl
	add	hl, hl
	ld	de,_keys
	add	hl, de
	push	bc
	push	hl
	call	_ht_lookup
	pop	af
	pop	bc
	bit	7, h
	jr	NZ,l_hash_compute_00118
	ld	e, l
	ld	d, h
	add	hl, hl
	add	hl, de
	ld	e,(ix-5)
	ld	d,(ix-4)
	add	hl, de
	inc	hl
	ld	(ix-5),l
	ld	(ix-4),h
l_hash_compute_00118:
	inc	bc
	ld	a, c
	sub	a,0xc8
	ld	a, b
	rla
	ccf
	rra
	sbc	a,0x80
	jr	C,l_hash_compute_00117
	inc	(ix-1)
	ld	a,(ix-1)
	sub	a,0x0e
	jp	C, l_hash_compute_00127
	ld	l,(ix-5)
	ld	h,(ix-4)
	ld	sp, ix
	pop	ix
	ret
;	---------------------------------
; Function hash_run
; ---------------------------------
_hash_run:
	call	_hash_compute
	ld	de,___str_1+0
	ld	bc,___str_0+0
	ld	a, l
	sub	a,0xe8
	jr	NZ,l_hash_run_00103
	ld	a, h
	sub	a,0x33
	jr	NZ,l_hash_run_00103
	ld	a,0x01
	jr	l_hash_run_00104
l_hash_run_00103:
	xor	a,a
l_hash_run_00104:
	ld	h,0x00
	push	de
	ld	de,0x0065
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
	DEFM "upstream/hashbench/hashbench.c"
	DEFB 0x00
	SECTION rodata_compiler
___str_1:
	DEFM "open-addressing hash table checksum (host-verified)"
	DEFB 0x00
	SECTION code_compiler
;	---------------------------------
; Function suite_hash
; ---------------------------------
_suite_hash:
	ld	hl,___str_2
	push	hl
	call	_suite_setup
	ld	hl,_hash_run
	ex	(sp),hl
	ld	hl,___str_3
	push	hl
	call	_suite_add_test_real
	pop	af
	pop	af
	jp	_suite_run
	SECTION rodata_compiler
___str_2:
	DEFM "Hash Table Tests"
	DEFB 0x00
	SECTION rodata_compiler
___str_3:
	DEFM "hash_run"
	DEFB 0x00
	SECTION code_compiler
;	---------------------------------
; Function main
; ---------------------------------
_main:
	call	_suite_hash
	jp	_exit_fastcall
	SECTION IGNORE
