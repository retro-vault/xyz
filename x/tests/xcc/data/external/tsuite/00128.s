	.module xcc_output

	.area _DATA
	.globl _a
_a:
	.ds 2
	.globl _b
_b:
	.ds 2
	.globl _c
_c:
	.ds 1
	.globl _d
_d:
	.ds 1
	.globl _e
_e:
	.ds 1
	.globl _f
_f:
	.ds 4
	.globl _g
_g:
	.ds 4
	.globl _h
_h:
	.ds 8
	.globl _i
_i:
	.ds 8
	.globl _j
_j:
	.ds 2
	.globl _k
_k:
	.ds 2


	.area _CODE

	.globl _main
_main:
	; prologue: main (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, (_b)
	ld	(_a), hl
	ld	a, (_c)
	ld	(_a), a
	ld	a, (_d)
	ld	(_a), a
	ld	a, (_e)
	ld	(_a), a
	ld	hl, (#_f)
	ld	(#_a), hl
	ld	hl, (#_f + 2)
	ld	(#_a + 2), hl
	ld	hl, (#_g)
	ld	(#_a), hl
	ld	hl, (#_g + 2)
	ld	(#_a + 2), hl
	ld	hl, (#_h)
	ld	(#_a), hl
	ld	hl, (#_h + 2)
	ld	(#_a + 2), hl
	ld	hl, (#_h + 4)
	ld	(#_a + 4), hl
	ld	hl, (#_h + 6)
	ld	(#_a + 6), hl
	ld	hl, (#_i)
	ld	(#_a), hl
	ld	hl, (#_i + 2)
	ld	(#_a + 2), hl
	ld	hl, (#_i + 4)
	ld	(#_a + 4), hl
	ld	hl, (#_i + 6)
	ld	(#_a + 6), hl
	ld	hl, (_j)
	ld	(_a), hl
	ld	hl, (_k)
	ld	(_a), hl
	ld	hl, (_a)
	ld	(_b), hl
	ld	a, (_c)
	ld	(_b), a
	ld	a, (_d)
	ld	(_b), a
	ld	a, (_e)
	ld	(_b), a
	ld	hl, (#_f)
	ld	(#_b), hl
	ld	hl, (#_f + 2)
	ld	(#_b + 2), hl
	ld	hl, (#_g)
	ld	(#_b), hl
	ld	hl, (#_g + 2)
	ld	(#_b + 2), hl
	ld	hl, (#_h)
	ld	(#_b), hl
	ld	hl, (#_h + 2)
	ld	(#_b + 2), hl
	ld	hl, (#_h + 4)
	ld	(#_b + 4), hl
	ld	hl, (#_h + 6)
	ld	(#_b + 6), hl
	ld	hl, (#_i)
	ld	(#_b), hl
	ld	hl, (#_i + 2)
	ld	(#_b + 2), hl
	ld	hl, (#_i + 4)
	ld	(#_b + 4), hl
	ld	hl, (#_i + 6)
	ld	(#_b + 6), hl
	ld	hl, (_j)
	ld	(_b), hl
	ld	hl, (_k)
	ld	(_b), hl
	ld	hl, (_a)
	ld	(_c), hl
	ld	hl, (_b)
	ld	(_c), hl
	ld	a, (_d)
	ld	(_c), a
	ld	a, (_e)
	ld	(_c), a
	ld	hl, (#_f)
	ld	(#_c), hl
	ld	hl, (#_f + 2)
	ld	(#_c + 2), hl
	ld	hl, (#_g)
	ld	(#_c), hl
	ld	hl, (#_g + 2)
	ld	(#_c + 2), hl
	ld	hl, (#_h)
	ld	(#_c), hl
	ld	hl, (#_h + 2)
	ld	(#_c + 2), hl
	ld	hl, (#_h + 4)
	ld	(#_c + 4), hl
	ld	hl, (#_h + 6)
	ld	(#_c + 6), hl
	ld	hl, (#_i)
	ld	(#_c), hl
	ld	hl, (#_i + 2)
	ld	(#_c + 2), hl
	ld	hl, (#_i + 4)
	ld	(#_c + 4), hl
	ld	hl, (#_i + 6)
	ld	(#_c + 6), hl
	ld	hl, (_j)
	ld	(_c), hl
	ld	hl, (_k)
	ld	(_c), hl
	ld	hl, (_a)
	ld	(_d), hl
	ld	hl, (_b)
	ld	(_d), hl
	ld	a, (_c)
	ld	(_d), a
	ld	a, (_e)
	ld	(_d), a
	ld	hl, (#_f)
	ld	(#_d), hl
	ld	hl, (#_f + 2)
	ld	(#_d + 2), hl
	ld	hl, (#_g)
	ld	(#_d), hl
	ld	hl, (#_g + 2)
	ld	(#_d + 2), hl
	ld	hl, (#_h)
	ld	(#_d), hl
	ld	hl, (#_h + 2)
	ld	(#_d + 2), hl
	ld	hl, (#_h + 4)
	ld	(#_d + 4), hl
	ld	hl, (#_h + 6)
	ld	(#_d + 6), hl
	ld	hl, (#_i)
	ld	(#_d), hl
	ld	hl, (#_i + 2)
	ld	(#_d + 2), hl
	ld	hl, (#_i + 4)
	ld	(#_d + 4), hl
	ld	hl, (#_i + 6)
	ld	(#_d + 6), hl
	ld	hl, (_j)
	ld	(_d), hl
	ld	hl, (_k)
	ld	(_d), hl
	ld	hl, (_a)
	ld	(_e), hl
	ld	hl, (_b)
	ld	(_e), hl
	ld	a, (_c)
	ld	(_e), a
	ld	a, (_d)
	ld	(_e), a
	ld	hl, (#_f)
	ld	(#_e), hl
	ld	hl, (#_f + 2)
	ld	(#_e + 2), hl
	ld	hl, (#_g)
	ld	(#_e), hl
	ld	hl, (#_g + 2)
	ld	(#_e + 2), hl
	ld	hl, (#_h)
	ld	(#_e), hl
	ld	hl, (#_h + 2)
	ld	(#_e + 2), hl
	ld	hl, (#_h + 4)
	ld	(#_e + 4), hl
	ld	hl, (#_h + 6)
	ld	(#_e + 6), hl
	ld	hl, (#_i)
	ld	(#_e), hl
	ld	hl, (#_i + 2)
	ld	(#_e + 2), hl
	ld	hl, (#_i + 4)
	ld	(#_e + 4), hl
	ld	hl, (#_i + 6)
	ld	(#_e + 6), hl
	ld	hl, (_j)
	ld	(_e), hl
	ld	hl, (_k)
	ld	(_e), hl
	ld	hl, (_a)
	ld	(_f), hl
	ld	hl, (_b)
	ld	(_f), hl
	ld	a, (_c)
	ld	(_f), a
	ld	a, (_d)
	ld	(_f), a
	ld	a, (_e)
	ld	(_f), a
	ld	hl, (#_g)
	ld	(#_f), hl
	ld	hl, (#_g + 2)
	ld	(#_f + 2), hl
	ld	hl, (#_h)
	ld	(#_f), hl
	ld	hl, (#_h + 2)
	ld	(#_f + 2), hl
	ld	hl, (#_h + 4)
	ld	(#_f + 4), hl
	ld	hl, (#_h + 6)
	ld	(#_f + 6), hl
	ld	hl, (#_i)
	ld	(#_f), hl
	ld	hl, (#_i + 2)
	ld	(#_f + 2), hl
	ld	hl, (#_i + 4)
	ld	(#_f + 4), hl
	ld	hl, (#_i + 6)
	ld	(#_f + 6), hl
	ld	hl, (_j)
	ld	(_f), hl
	ld	hl, (_k)
	ld	(_f), hl
	ld	hl, (_a)
	ld	(_g), hl
	ld	hl, (_b)
	ld	(_g), hl
	ld	a, (_c)
	ld	(_g), a
	ld	a, (_d)
	ld	(_g), a
	ld	a, (_e)
	ld	(_g), a
	ld	hl, (#_f)
	ld	(#_g), hl
	ld	hl, (#_f + 2)
	ld	(#_g + 2), hl
	ld	hl, (#_h)
	ld	(#_g), hl
	ld	hl, (#_h + 2)
	ld	(#_g + 2), hl
	ld	hl, (#_h + 4)
	ld	(#_g + 4), hl
	ld	hl, (#_h + 6)
	ld	(#_g + 6), hl
	ld	hl, (#_i)
	ld	(#_g), hl
	ld	hl, (#_i + 2)
	ld	(#_g + 2), hl
	ld	hl, (#_i + 4)
	ld	(#_g + 4), hl
	ld	hl, (#_i + 6)
	ld	(#_g + 6), hl
	ld	hl, (_j)
	ld	(_g), hl
	ld	hl, (_k)
	ld	(_g), hl
	ld	hl, (_a)
	ld	(_h), hl
	ld	hl, (_b)
	ld	(_h), hl
	ld	a, (_c)
	ld	(_h), a
	ld	a, (_d)
	ld	(_h), a
	ld	a, (_e)
	ld	(_h), a
	ld	hl, (#_f)
	ld	(#_h), hl
	ld	hl, (#_f + 2)
	ld	(#_h + 2), hl
	ld	hl, (#_g)
	ld	(#_h), hl
	ld	hl, (#_g + 2)
	ld	(#_h + 2), hl
	ld	hl, (#_i)
	ld	(#_h), hl
	ld	hl, (#_i + 2)
	ld	(#_h + 2), hl
	ld	hl, (#_i + 4)
	ld	(#_h + 4), hl
	ld	hl, (#_i + 6)
	ld	(#_h + 6), hl
	ld	hl, (_j)
	ld	(_h), hl
	ld	hl, (_k)
	ld	(_h), hl
	ld	hl, (_a)
	ld	(_i), hl
	ld	hl, (_b)
	ld	(_i), hl
	ld	a, (_c)
	ld	(_i), a
	ld	a, (_d)
	ld	(_i), a
	ld	a, (_e)
	ld	(_i), a
	ld	hl, (#_f)
	ld	(#_i), hl
	ld	hl, (#_f + 2)
	ld	(#_i + 2), hl
	ld	hl, (#_g)
	ld	(#_i), hl
	ld	hl, (#_g + 2)
	ld	(#_i + 2), hl
	ld	hl, (#_h)
	ld	(#_i), hl
	ld	hl, (#_h + 2)
	ld	(#_i + 2), hl
	ld	hl, (#_h + 4)
	ld	(#_i + 4), hl
	ld	hl, (#_h + 6)
	ld	(#_i + 6), hl
	ld	hl, (_j)
	ld	(_i), hl
	ld	hl, (_k)
	ld	(_i), hl
	ld	hl, (_a)
	ld	(_j), hl
	ld	hl, (_b)
	ld	(_j), hl
	ld	a, (_c)
	ld	(_j), a
	ld	a, (_d)
	ld	(_j), a
	ld	a, (_e)
	ld	(_j), a
	ld	hl, (#_f)
	ld	(#_j), hl
	ld	hl, (#_f + 2)
	ld	(#_j + 2), hl
	ld	hl, (#_g)
	ld	(#_j), hl
	ld	hl, (#_g + 2)
	ld	(#_j + 2), hl
	ld	hl, (#_h)
	ld	(#_j), hl
	ld	hl, (#_h + 2)
	ld	(#_j + 2), hl
	ld	hl, (#_h + 4)
	ld	(#_j + 4), hl
	ld	hl, (#_h + 6)
	ld	(#_j + 6), hl
	ld	hl, (#_i)
	ld	(#_j), hl
	ld	hl, (#_i + 2)
	ld	(#_j + 2), hl
	ld	hl, (#_i + 4)
	ld	(#_j + 4), hl
	ld	hl, (#_i + 6)
	ld	(#_j + 6), hl
	ld	hl, (_k)
	ld	(_j), hl
	ld	hl, (_a)
	ld	(_k), hl
	ld	hl, (_b)
	ld	(_k), hl
	ld	a, (_c)
	ld	(_k), a
	ld	a, (_d)
	ld	(_k), a
	ld	a, (_e)
	ld	(_k), a
	ld	hl, (#_f)
	ld	(#_k), hl
	ld	hl, (#_f + 2)
	ld	(#_k + 2), hl
	ld	hl, (#_g)
	ld	(#_k), hl
	ld	hl, (#_g + 2)
	ld	(#_k + 2), hl
	ld	hl, (#_h)
	ld	(#_k), hl
	ld	hl, (#_h + 2)
	ld	(#_k + 2), hl
	ld	hl, (#_h + 4)
	ld	(#_k + 4), hl
	ld	hl, (#_h + 6)
	ld	(#_k + 6), hl
	ld	hl, (_j)
	ld	(_k), hl
	ld	hl, (#_i)
	ld	(#_k), hl
	ld	hl, (#_i + 2)
	ld	(#_k + 2), hl
	ld	hl, (#_i + 4)
	ld	(#_k + 4), hl
	ld	hl, (#_i + 6)
	ld	(#_k + 6), hl
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
