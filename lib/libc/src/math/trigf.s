        ;; trigf.s
        ;;
        ;; libc sinf / cosf / tanf for the xcc Z80 libc.
        ;; Range reduction uses q = round(x * 2/pi), then evaluates small-angle
        ;; Taylor polynomials on r = x - q*(pi/2), where |r| <= pi/4.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module trigf
        .optsdcc -mz80 sdcccall(1)

        .globl  _sinf
        .globl  _cosf
        .globl  _tanf
        .globl  ___libc_fpclassifyf
        .globl  ___fsmul
        .globl  ___fsadd
        .globl  ___fssub
        .globl  ___fsdiv
        .globl  _roundf
        .globl  ___fs2slong

        .area   _DATA
__tg_x:   .ds 4
__tg_qf:  .ds 4
__tg_qi:  .ds 4
__tg_r:   .ds 4
__tg_x2:  .ds 4
__tg_u:   .ds 4
__tg_s:   .ds 4
__tg_c:   .ds 4

        .area   _CODE

__tg_ret_x:
        ld      hl,(__tg_x + 2)
        ld      de,(__tg_x)
        ret

__tg_ret_nan:
        ld      hl,#0x7fc0
        ld      de,#0x0000
        ret

__tg_neg_hlde:
        ld      a,h
        xor     #0x80
        ld      h,a
        ret

        ;; __tg_reduce
        ;; input:  __tg_x already filled
        ;; output: __tg_qf, __tg_qi, __tg_r
        ;;         A = quadrant = q mod 4
__tg_reduce:
        ;; qf = roundf(x * 2/pi)
        ld      hl,#0x3f22              ; 2/pi
        push    hl
        ld      hl,#0xf983
        push    hl
        ld      de,(__tg_x)
        ld      hl,(__tg_x + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        call    _roundf
        ld      (__tg_qf),de
        ld      (__tg_qf + 2),hl

        ;; qi = (long)qf
        call    ___fs2slong
        ld      (__tg_qi),de
        ld      (__tg_qi + 2),hl

        ;; r = x - qf * (pi/2)
        ld      hl,#0x3fc9              ; pi/2
        push    hl
        ld      hl,#0x0fdb
        push    hl
        ld      de,(__tg_qf)
        ld      hl,(__tg_qf + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__tg_s),de             ; temporary product slot
        ld      (__tg_s + 2),hl
        ld      hl,(__tg_s + 2)
        push    hl
        ld      hl,(__tg_s)
        push    hl
        ld      de,(__tg_x)
        ld      hl,(__tg_x + 2)
        call    ___fssub
        pop     bc
        pop     bc
        ld      (__tg_r),de
        ld      (__tg_r + 2),hl

        ld      a,(__tg_qi)
        and     #3
        ret

        ;; __tg_square_r
        ;; computes __tg_x2 = __tg_r * __tg_r
__tg_square_r:
        ld      hl,(__tg_r + 2)
        push    hl
        ld      hl,(__tg_r)
        push    hl
        ld      de,(__tg_r)
        ld      hl,(__tg_r + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__tg_x2),de
        ld      (__tg_x2 + 2),hl
        ret

        ;; __tg_sinpoly
        ;; returns sin(__tg_r) in HL:DE for |r| <= pi/4
__tg_sinpoly:
        call    __tg_square_r

        ;; u = c7*x2 + c5
        ld      hl,#0xb950              ; -1/5040
        push    hl
        ld      hl,#0x0d01
        push    hl
        ld      de,(__tg_x2)
        ld      hl,(__tg_x2 + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__tg_u),de
        ld      (__tg_u + 2),hl
        ld      hl,#0x3c08              ; +1/120
        push    hl
        ld      hl,#0x8889
        push    hl
        ld      de,(__tg_u)
        ld      hl,(__tg_u + 2)
        call    ___fsadd
        pop     bc
        pop     bc
        ld      (__tg_u),de
        ld      (__tg_u + 2),hl

        ;; u = u*x2 + c3
        ld      hl,(__tg_x2 + 2)
        push    hl
        ld      hl,(__tg_x2)
        push    hl
        ld      de,(__tg_u)
        ld      hl,(__tg_u + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__tg_u),de
        ld      (__tg_u + 2),hl
        ld      hl,#0xbe2a              ; -1/6
        push    hl
        ld      hl,#0xaaab
        push    hl
        ld      de,(__tg_u)
        ld      hl,(__tg_u + 2)
        call    ___fsadd
        pop     bc
        pop     bc
        ld      (__tg_u),de
        ld      (__tg_u + 2),hl

        ;; u = 1 + u*x2
        ld      hl,(__tg_x2 + 2)
        push    hl
        ld      hl,(__tg_x2)
        push    hl
        ld      de,(__tg_u)
        ld      hl,(__tg_u + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__tg_u),de
        ld      (__tg_u + 2),hl
        ld      hl,#0x3f80              ; +1.0
        push    hl
        ld      hl,#0x0000
        push    hl
        ld      de,(__tg_u)
        ld      hl,(__tg_u + 2)
        call    ___fsadd
        pop     bc
        pop     bc
        ld      (__tg_u),de
        ld      (__tg_u + 2),hl

        ;; result = r * u
        ld      hl,(__tg_u + 2)
        push    hl
        ld      hl,(__tg_u)
        push    hl
        ld      de,(__tg_r)
        ld      hl,(__tg_r + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        ret

        ;; __tg_cospoly
        ;; returns cos(__tg_r) in HL:DE for |r| <= pi/4
__tg_cospoly:
        call    __tg_square_r

        ;; u = c6*x2 + c4
        ld      hl,#0xbab6              ; -1/720
        push    hl
        ld      hl,#0x0b61
        push    hl
        ld      de,(__tg_x2)
        ld      hl,(__tg_x2 + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__tg_u),de
        ld      (__tg_u + 2),hl
        ld      hl,#0x3d2a              ; +1/24
        push    hl
        ld      hl,#0xaaab
        push    hl
        ld      de,(__tg_u)
        ld      hl,(__tg_u + 2)
        call    ___fsadd
        pop     bc
        pop     bc
        ld      (__tg_u),de
        ld      (__tg_u + 2),hl

        ;; u = u*x2 + c2
        ld      hl,(__tg_x2 + 2)
        push    hl
        ld      hl,(__tg_x2)
        push    hl
        ld      de,(__tg_u)
        ld      hl,(__tg_u + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__tg_u),de
        ld      (__tg_u + 2),hl
        ld      hl,#0xbf00              ; -1/2
        push    hl
        ld      hl,#0x0000
        push    hl
        ld      de,(__tg_u)
        ld      hl,(__tg_u + 2)
        call    ___fsadd
        pop     bc
        pop     bc
        ld      (__tg_u),de
        ld      (__tg_u + 2),hl

        ;; result = 1 + u*x2
        ld      hl,(__tg_x2 + 2)
        push    hl
        ld      hl,(__tg_x2)
        push    hl
        ld      de,(__tg_u)
        ld      hl,(__tg_u + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__tg_u),de
        ld      (__tg_u + 2),hl
        ld      hl,#0x3f80              ; +1.0
        push    hl
        ld      hl,#0x0000
        push    hl
        ld      de,(__tg_u)
        ld      hl,(__tg_u + 2)
        call    ___fsadd
        pop     bc
        pop     bc
        ret

        ;; HL:DE = x -> HL:DE = sin(x)
_sinf::
        ld      (__tg_x),de
        ld      (__tg_x + 2),hl
        call    ___libc_fpclassifyf
        ld      a,e
        cp      #2
        jp      c,__tg_ret_nan
        jp      z,__tg_ret_x
        ld      de,(__tg_x)
        ld      hl,(__tg_x + 2)
        call    __tg_reduce
        cp      #1
        jr      z,tg_sin_q1
        cp      #2
        jr      z,tg_sin_q2
        cp      #3
        jr      z,tg_sin_q3
        jp      __tg_sinpoly
tg_sin_q1:
        jp      __tg_cospoly
tg_sin_q2:
        call    __tg_sinpoly
        jp      __tg_neg_hlde
tg_sin_q3:
        call    __tg_cospoly
        jp      __tg_neg_hlde

        ;; HL:DE = x -> HL:DE = cos(x)
_cosf::
        ld      (__tg_x),de
        ld      (__tg_x + 2),hl
        call    ___libc_fpclassifyf
        ld      a,e
        cp      #2
        jp      c,__tg_ret_nan
        jr      z,tg_cos_zero
        ld      de,(__tg_x)
        ld      hl,(__tg_x + 2)
        call    __tg_reduce
        cp      #1
        jr      z,tg_cos_q1
        cp      #2
        jr      z,tg_cos_q2
        cp      #3
        jr      z,tg_cos_q3
        jp      __tg_cospoly
tg_cos_q1:
        call    __tg_sinpoly
        jp      __tg_neg_hlde
tg_cos_q2:
        call    __tg_cospoly
        jp      __tg_neg_hlde
tg_cos_q3:
        jp      __tg_sinpoly
tg_cos_zero:
        ld      hl,#0x3f80
        ld      de,#0x0000
        ret

        ;; HL:DE = x -> HL:DE = tan(x)
_tanf::
        ld      (__tg_x),de
        ld      (__tg_x + 2),hl
        call    ___libc_fpclassifyf
        ld      a,e
        cp      #2
        jp      c,__tg_ret_nan
        jp      z,__tg_ret_x
        ld      de,(__tg_x)
        ld      hl,(__tg_x + 2)
        call    __tg_reduce
        call    __tg_sinpoly
        ld      (__tg_s),de
        ld      (__tg_s + 2),hl
        call    __tg_cospoly
        ld      (__tg_c),de
        ld      (__tg_c + 2),hl
        ld      a,(__tg_qi)
        and     #1
        jr      nz,tg_tan_odd
        ld      hl,(__tg_c + 2)
        push    hl
        ld      hl,(__tg_c)
        push    hl
        ld      de,(__tg_s)
        ld      hl,(__tg_s + 2)
        call    ___fsdiv
        pop     bc
        pop     bc
        ret
tg_tan_odd:
        ld      hl,(__tg_s + 2)
        push    hl
        ld      hl,(__tg_s)
        push    hl
        ld      de,(__tg_c)
        ld      hl,(__tg_c + 2)
        call    ___fsdiv
        pop     bc
        pop     bc
        jp      __tg_neg_hlde
