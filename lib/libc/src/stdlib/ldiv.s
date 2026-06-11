        ;; ldiv.s
        ;;
        ;; libc ldiv() for the xcc Z80 libc.
        ;; Computes quotient and remainder for two signed 32-bit longs and
        ;; returns ldiv_t as an 8-byte aggregate in the normal 64-bit
        ;; DE:HL:DE':HL' register layout.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module ldiv
        .optsdcc -mz80 sdcccall(1)

        .globl  _ldiv
        .globl  __sdiv32
        .globl  __smod32

        .area   _CODE

_ldiv::
        ;; First arg numer arrives in DE:HL. The second long denom is stacked
        ;; at ix+4..7 (low word first) under sdcccall(1).
        push    ix
        ld      ix,#0
        add     ix,sp
        ;;
        ;; Match the compiler's sdcccall(1) frame shape for mixed
        ;; register/stack-parameter functions: the register-passed first
        ;; argument is homed in the top four bytes below IX before the local
        ;; frame is extended.
        ld      -4(ix),e
        ld      -3(ix),d
        ld      -2(ix),l
        ld      -1(ix),h
        ld      hl,#-8
        add     hl,sp
        ld      sp,hl

        ld      l,6(ix)
        ld      h,7(ix)
        push    hl
        ld      l,4(ix)
        ld      h,5(ix)
        push    hl                      ; denom low
        ld      l,-4(ix)
        ld      h,-3(ix)
        push    hl
        ld      l,-2(ix)
        ld      h,-1(ix)
        pop     de
        call    __sdiv32                ; DE:HL = quotient (lo:hi)
        pop     bc
        pop     bc
        ld      b,h
        ld      c,l
        ex      de,hl
        ld      -8(ix),l
        ld      -7(ix),h               ; quotient low
        ld      h,b
        ld      l,c
        ld      -6(ix),l
        ld      -5(ix),h               ; quotient high

        ld      l,6(ix)
        ld      h,7(ix)
        push    hl
        ld      l,4(ix)
        ld      h,5(ix)
        push    hl
        ld      l,-4(ix)
        ld      h,-3(ix)
        push    hl
        ld      l,-2(ix)
        ld      h,-1(ix)
        pop     de
        call    __smod32                ; DE:HL = remainder (lo:hi)
        pop     bc
        pop     bc

        push    hl                      ; rem high
        ex      de,hl
        push    hl                      ; rem low
        ld      l,-6(ix)
        ld      h,-5(ix)
        push    hl                      ; quot high
        ld      l,-8(ix)
        ld      h,-7(ix)
        push    hl                      ; quot low
        pop     de
        pop     hl
        exx
        pop     de
        pop     hl
        exx
        ld      sp,ix
        pop     ix
        ret
