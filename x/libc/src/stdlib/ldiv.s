        ;; ldiv.s
        ;;
        ;; libc ldiv() for the xcc Z80 libc.
        ;; Computes quotient and remainder for two signed 32-bit longs and
        ;; writes ldiv_t to the SDCC hidden aggregate-result pointer.
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
        ;; at ix+6..9 (low word first) under sdcccall(1); the hidden result
        ;; pointer occupies ix+4..5.
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

        ld      l,8(ix)
        ld      h,9(ix)
        push    hl
        ld      l,6(ix)
        ld      h,7(ix)
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

        ld      l,8(ix)
        ld      h,9(ix)
        push    hl
        ld      l,6(ix)
        ld      h,7(ix)
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

        ld      c,4(ix)
        ld      b,5(ix)                 ; BC = caller result
        ld      a,-8(ix)
        ld      (bc),a                  ; quotient low to high
        inc     bc
        ld      a,-7(ix)
        ld      (bc),a
        inc     bc
        ld      a,-6(ix)
        ld      (bc),a
        inc     bc
        ld      a,-5(ix)
        ld      (bc),a
        inc     bc
        ld      a,e
        ld      (bc),a                  ; remainder low to high
        inc     bc
        ld      a,d
        ld      (bc),a
        inc     bc
        ld      a,l
        ld      (bc),a
        inc     bc
        ld      a,h
        ld      (bc),a
        ld      sp,ix
        pop     ix
        ret
