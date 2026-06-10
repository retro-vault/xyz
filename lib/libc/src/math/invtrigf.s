        ;; invtrigf.s
        ;;
        ;; libc atanf / asinf / acosf for the xcc Z80 libc.
        ;; Built on the existing atan2f / sqrtf kernels plus the float runtime.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module invtrigf
        .optsdcc -mz80 sdcccall(1)

        .globl  _atanf
        .globl  _asinf
        .globl  _acosf
        .globl  _atan2f
        .globl  _sqrtf
        .globl  ___fsmul
        .globl  ___fssub

        .area   _DATA
__it_x: .ds 4
__it_t: .ds 4

        .area   _CODE

        ;; atanf(x) is routed through atan2f(x, 1.0f) so the quadrant logic
        ;; stays in one proven kernel.
_atanf::
        ld      (__it_x),de
        ld      (__it_x + 2),hl
        ld      hl,#0x3f80              ; 1.0f high word
        push    hl
        ld      hl,#0x0000              ; 1.0f low word
        push    hl
        ld      de,(__it_x)
        ld      hl,(__it_x + 2)
        call    _atan2f
        pop     bc
        pop     bc
        ret

        ;; asinf(x) = atan2f(x, sqrtf(1 - x*x))
_asinf::
        ld      (__it_x),de
        ld      (__it_x + 2),hl
        ;; Form x*x first so the domain check naturally falls out of sqrtf.
        ld      hl,(__it_x + 2)
        ld      bc,(__it_x)
        push    hl
        push    bc
        ld      de,(__it_x)
        ld      hl,(__it_x + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__it_t),de
        ld      (__it_t + 2),hl
        ;; Transform to 1 - x*x for the sqrtf leg of the identity.
        ld      hl,(__it_t + 2)
        ld      bc,(__it_t)
        push    hl
        push    bc
        ld      de,#0x0000
        ld      hl,#0x3f80              ; 1.0f
        call    ___fssub
        pop     bc
        pop     bc
        call    _sqrtf
        ;; atan2f takes y in HL:DE and x on the stack.
        push    hl
        push    de
        ld      de,(__it_x)
        ld      hl,(__it_x + 2)
        call    _atan2f
        pop     bc
        pop     bc
        ret

        ;; acosf(x) = atan2f(sqrtf(1 - x*x), x)
_acosf::
        ld      (__it_x),de
        ld      (__it_x + 2),hl
        ;; Reuse the same 1 - x*x reduction as asinf.
        ld      hl,(__it_x + 2)
        ld      bc,(__it_x)
        push    hl
        push    bc
        ld      de,(__it_x)
        ld      hl,(__it_x + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__it_t),de
        ld      (__it_t + 2),hl
        ld      hl,(__it_t + 2)
        ld      bc,(__it_t)
        push    hl
        push    bc
        ld      de,#0x0000
        ld      hl,#0x3f80              ; 1.0f
        call    ___fssub
        pop     bc
        pop     bc
        call    _sqrtf
        ld      (__it_t),de
        ld      (__it_t + 2),hl
        ;; For acosf the reduced root becomes y and the original x is stacked.
        ld      hl,(__it_x + 2)
        ld      bc,(__it_x)
        push    hl
        push    bc
        ld      de,(__it_t)
        ld      hl,(__it_t + 2)
        call    _atan2f
        pop     bc
        pop     bc
        ret
