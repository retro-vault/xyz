        ;; atan2f.s
        ;;
        ;; libc atan2f for the xcc Z80 libc.  Rational approximation built on the
        ;; soft-float runtime.  Matches the
        ;; previous C implementation (0.28 Pade-style fit), including the
        ;; quadrant corrections.  Constants:
        ;;   PI = 0x40490FDB  0.5*PI = 0x3FC90FDB  0.28 = 0x3E8F5C29
        ;;   1.0 = 0x3F800000
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module atan2f
        .optsdcc -mz80 sdcccall(1)
        .globl  _atan2f
	.globl  ___libc_fpclassifyf
	.globl  ___fsadd
	.globl  ___fssub
	.globl  ___fsmul
	.globl  ___fsdiv

AT_YLO  .equ    -20
AT_YHI  .equ    -18
AT_XLO  .equ    -16
AT_XHI  .equ    -14
AT_ZLO  .equ    -12
AT_ZHI  .equ    -10
AT_TLO  .equ    -8
AT_THI  .equ    -6
AT_ALO  .equ    -4
AT_AHI  .equ    -2
	.area   _CODE
	;; HL:DE = y, x at 4(ix)..7(ix) -> HL:DE = atan2(y, x)
_atan2f::
	push    ix
	ld      ix,#0
	add     ix,sp
	ld      c,l
	ld      b,h
	ld      hl,#-20
	add     hl,sp
	ld      sp,hl
	ld      AT_YLO(ix),e
	ld      AT_YLO+1(ix),d
	ld      AT_YHI(ix),c
	ld      AT_YHI+1(ix),b
	ld      a,4(ix)
	ld      AT_XLO(ix),a
	ld      a,5(ix)
	ld      AT_XLO+1(ix),a
	ld      a,6(ix)
	ld      AT_XHI(ix),a
	ld      a,7(ix)
	ld      AT_XHI+1(ix),a
	;; NaN check
	ld      l,AT_XHI(ix)
	ld      h,AT_XHI+1(ix)
	ld      e,AT_XLO(ix)
	ld      d,AT_XLO+1(ix)
	call    ___libc_fpclassifyf
	ld      a,d
	or      e
	jp      z,at_nan
	ld      l,AT_YHI(ix)
	ld      h,AT_YHI+1(ix)
	ld      e,AT_YLO(ix)
	ld      d,AT_YLO+1(ix)
	call    ___libc_fpclassifyf
	ld      a,d
	or      e
	jp      z,at_nan
	;; x == 0 ?
	ld      a,AT_XHI+1(ix)
	and     #0x7f
	ld      c,a
	ld      a,AT_XHI(ix)
	or      c
	ld      c,a
	ld      a,AT_XLO+1(ix)
	or      c
	ld      c,a
	ld      a,AT_XLO(ix)
	or      c
	jp      nz,at_xnonzero
	;; y == 0 ?
	ld      a,AT_YHI+1(ix)
	and     #0x7f
	ld      c,a
	ld      a,AT_YHI(ix)
	or      c
	ld      c,a
	ld      a,AT_YLO+1(ix)
	or      c
	ld      c,a
	ld      a,AT_YLO(ix)
	or      c
	jr      z,at_zero
	ld      a,AT_YHI+1(ix)
	bit     7,a
	jr      nz,at_neg_hpi
	ld      hl,#0x3fc9              ; +0.5*PI
        ld      de,#0x0fdb
        jp      at_ret
at_neg_hpi:
        ld      hl,#0xbfc9              ; -0.5*PI
        ld      de,#0x0fdb
        jp      at_ret
at_zero:
        ld      hl,#0
        ld      de,#0
        jp      at_ret
at_nan:
        ld      hl,#0x7fc0
        ld      de,#0
	jp      at_ret
at_xnonzero:
	;; z = y / x
	ld      l,AT_XHI(ix)
	ld      h,AT_XHI+1(ix)
	ld      c,AT_XLO(ix)
	ld      b,AT_XLO+1(ix)
	push    hl
	push    bc
	ld      e,AT_YLO(ix)
	ld      d,AT_YLO+1(ix)
	ld      l,AT_YHI(ix)
	ld      h,AT_YHI+1(ix)
	call    ___fsdiv
	pop     bc
	pop     bc
	ld      AT_ZLO(ix),e
	ld      AT_ZLO+1(ix),d
	ld      AT_ZHI(ix),l
	ld      AT_ZHI+1(ix),h
	;; abs_z < 1.0 ? e8(z) < 127
	ld      a,AT_ZHI+1(ix)
	and     #0x7f
	add     a,a
	ld      c,a
	ld      a,AT_ZHI(ix)
	rlca
	and     #1
	or      c
	cp      #127
	jp      nc,at_large
	;; small: atan = z / (1.0 + 0.28*z*z)
	ld      l,AT_ZHI(ix)
	ld      h,AT_ZHI+1(ix)
	ld      c,AT_ZLO(ix)
	ld      b,AT_ZLO+1(ix)
	push    hl
	push    bc
	ld      e,AT_ZLO(ix)
	ld      d,AT_ZLO+1(ix)
	ld      l,AT_ZHI(ix)
	ld      h,AT_ZHI+1(ix)
	call    ___fsmul                ; z*z
	pop     bc
	pop     bc
	ld      AT_TLO(ix),e
	ld      AT_TLO+1(ix),d
	ld      AT_THI(ix),l
	ld      AT_THI+1(ix),h
	ld      l,AT_THI(ix)
	ld      h,AT_THI+1(ix)
	ld      c,AT_TLO(ix)
	ld      b,AT_TLO+1(ix)
	push    hl
	push    bc
	ld      de,#0x5c29              ; 0.28
	ld      hl,#0x3e8f
	call    ___fsmul                ; 0.28*z*z
	pop     bc
	pop     bc
	ld      AT_TLO(ix),e
	ld      AT_TLO+1(ix),d
	ld      AT_THI(ix),l
	ld      AT_THI+1(ix),h
	ld      l,AT_THI(ix)
	ld      h,AT_THI+1(ix)
	ld      c,AT_TLO(ix)
	ld      b,AT_TLO+1(ix)
	push    hl
	push    bc
	ld      de,#0x0000              ; 1.0
        ld      hl,#0x3f80
	call    ___fsadd                ; 1.0 + 0.28*z*z
	pop     bc
	pop     bc
	ld      AT_TLO(ix),e
	ld      AT_TLO+1(ix),d
	ld      AT_THI(ix),l
	ld      AT_THI+1(ix),h
	ld      l,AT_THI(ix)
	ld      h,AT_THI+1(ix)
	ld      c,AT_TLO(ix)
	ld      b,AT_TLO+1(ix)
	push    hl
	push    bc
	ld      e,AT_ZLO(ix)
	ld      d,AT_ZLO+1(ix)
	ld      l,AT_ZHI(ix)
	ld      h,AT_ZHI+1(ix)
	call    ___fsdiv                ; z / denom
	pop     bc
	pop     bc
	ld      AT_ALO(ix),e
	ld      AT_ALO+1(ix),d
	ld      AT_AHI(ix),l
	ld      AT_AHI+1(ix),h
	;; x<0 correction
	ld      a,AT_XHI+1(ix)
	bit     7,a
	jr      z,at_a_plain
	ld      a,AT_YHI+1(ix)
	bit     7,a
	jr      nz,at_a_subpi
	ld      hl,#0x4049              ; atan += PI
	ld      bc,#0x0fdb
	push    hl
	push    bc
	ld      e,AT_ALO(ix)
	ld      d,AT_ALO+1(ix)
	ld      l,AT_AHI(ix)
	ld      h,AT_AHI+1(ix)
	call    ___fsadd
	pop     bc
	pop     bc
	jp      at_ret
at_a_subpi:
	ld      hl,#0x4049              ; atan -= PI
	ld      bc,#0x0fdb
	push    hl
	push    bc
	ld      e,AT_ALO(ix)
	ld      d,AT_ALO+1(ix)
	ld      l,AT_AHI(ix)
	ld      h,AT_AHI+1(ix)
	call    ___fssub
	pop     bc
	pop     bc
	jp      at_ret
at_a_plain:
	ld      l,AT_AHI(ix)
	ld      h,AT_AHI+1(ix)
	ld      e,AT_ALO(ix)
	ld      d,AT_ALO+1(ix)
	jp      at_ret
at_large:
	;; atan = 0.5*PI - z/(z*z + 0.28); if y<0 atan -= PI
	ld      l,AT_ZHI(ix)
	ld      h,AT_ZHI+1(ix)
	ld      c,AT_ZLO(ix)
	ld      b,AT_ZLO+1(ix)
	push    hl
	push    bc
	ld      e,AT_ZLO(ix)
	ld      d,AT_ZLO+1(ix)
	ld      l,AT_ZHI(ix)
	ld      h,AT_ZHI+1(ix)
	call    ___fsmul                ; z*z
	pop     bc
	pop     bc
	ld      AT_TLO(ix),e
	ld      AT_TLO+1(ix),d
	ld      AT_THI(ix),l
	ld      AT_THI+1(ix),h
	ld      hl,#0x3e8f              ; + 0.28
	ld      bc,#0x5c29
	push    hl
	push    bc
	ld      e,AT_TLO(ix)
	ld      d,AT_TLO+1(ix)
	ld      l,AT_THI(ix)
	ld      h,AT_THI+1(ix)
	call    ___fsadd
	pop     bc
	pop     bc
	ld      AT_TLO(ix),e
	ld      AT_TLO+1(ix),d
	ld      AT_THI(ix),l
	ld      AT_THI+1(ix),h
	ld      l,AT_THI(ix)
	ld      h,AT_THI+1(ix)
	ld      c,AT_TLO(ix)
	ld      b,AT_TLO+1(ix)
	push    hl
	push    bc
	ld      e,AT_ZLO(ix)
	ld      d,AT_ZLO+1(ix)
	ld      l,AT_ZHI(ix)
	ld      h,AT_ZHI+1(ix)
	call    ___fsdiv                ; z / (z*z+0.28)
	pop     bc
	pop     bc
	ld      AT_TLO(ix),e
	ld      AT_TLO+1(ix),d
	ld      AT_THI(ix),l
	ld      AT_THI+1(ix),h
	ld      l,AT_THI(ix)
	ld      h,AT_THI+1(ix)
	ld      c,AT_TLO(ix)
	ld      b,AT_TLO+1(ix)
	push    hl
	push    bc
	ld      de,#0x0fdb              ; 0.5*PI
	ld      hl,#0x3fc9
	call    ___fssub                ; 0.5*PI - t
	pop     bc
	pop     bc
	ld      AT_ALO(ix),e
	ld      AT_ALO+1(ix),d
	ld      AT_AHI(ix),l
	ld      AT_AHI+1(ix),h
	ld      a,AT_YHI+1(ix)
	bit     7,a
	jr      z,at_large_plain
	ld      hl,#0x4049              ; atan -= PI
	ld      bc,#0x0fdb
	push    hl
	push    bc
	ld      e,AT_ALO(ix)
	ld      d,AT_ALO+1(ix)
	ld      l,AT_AHI(ix)
	ld      h,AT_AHI+1(ix)
	call    ___fssub
	pop     bc
	pop     bc
	jp      at_ret
at_large_plain:
	ld      l,AT_AHI(ix)
	ld      h,AT_AHI+1(ix)
	ld      e,AT_ALO(ix)
	ld      d,AT_ALO+1(ix)
at_ret:
	ld      sp,ix
	pop     ix
	ret
