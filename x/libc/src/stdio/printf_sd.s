        ;; Compact printf path selected for constant formats containing only
        ;; %s, %d, %i and %%.  The compiler retains the normal stack-only
        ;; variadic ABI, so this helper works in both sdcccall modes.

        .module printf_sd
        .optsdcc -mz80 sdcccall(0)

        .globl  ___printf_sd
        .globl  __stdio_alloc_ctx
        .globl  __stdio_emit_a
        .globl  __stdio_init_console
        .globl  __stdio_load_count_hl

        .area   _CODE
___printf_sd::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __stdio_alloc_ctx
        call    __stdio_init_console

        ld      c,4(ix)                 ; BC = format
        ld      b,5(ix)
        push    ix
        pop     hl
        ld      de,#6
        add     hl,de
        ex      de,hl                   ; DE = first variadic argument

__printf_sd_loop:
        ld      a,(bc)
        inc     bc
        or      a
        jr      z,__printf_sd_done
        cp      #'%'
        jr      nz,__printf_sd_emit

        ld      a,(bc)
        inc     bc
        cp      #'%'
        jr      z,__printf_sd_emit
        cp      #'s'
        jr      z,__printf_sd_string

        ;; %d and %i consume one promoted 16-bit int.
        push    bc
        ld      a,(de)
        ld      l,a
        inc     de
        ld      a,(de)
        ld      h,a
        inc     de
        push    de
        call    __printf_sd_signed
        pop     de
        pop     bc
        jr      __printf_sd_loop

__printf_sd_string:
        ld      a,(de)
        ld      l,a
        inc     de
        ld      a,(de)
        ld      h,a
        inc     de
__printf_sd_string_loop:
        ld      a,(hl)
        inc     hl
        or      a
        jr      z,__printf_sd_loop
        call    __stdio_emit_a
        jr      __printf_sd_string_loop

__printf_sd_emit:
        call    __stdio_emit_a
        jr      __printf_sd_loop

__printf_sd_done:
        call    __stdio_load_count_hl
        push    hl
        pop     de
        ld      sp,ix
        pop     ix
        ret

        ;; Emit a signed 16-bit decimal without linking division helpers.
__printf_sd_signed:
        bit     7,h
        jr      z,__printf_sd_unsigned
        ld      a,#'-'
        call    __stdio_emit_a
        xor     a
        sub     l
        ld      l,a
        sbc     a,a
        sub     h
        ld      h,a

__printf_sd_unsigned:
        ld      b,#0                    ; no nonzero digit emitted yet
        ld      de,#10000
        call    __printf_sd_place
        ld      de,#1000
        call    __printf_sd_place
        ld      de,#100
        call    __printf_sd_place
        ld      de,#10
        call    __printf_sd_place
        ld      a,l
        add     a,#'0'
        jp      __stdio_emit_a

__printf_sd_place:
        ld      c,#'0'
__printf_sd_place_loop:
        or      a
        sbc     hl,de
        jr      c,__printf_sd_place_restore
        inc     c
        jr      __printf_sd_place_loop
__printf_sd_place_restore:
        add     hl,de
        ld      a,b
        or      a
        jr      nz,__printf_sd_place_emit
        ld      a,c
        cp      #'0'
        ret     z
        ld      b,#1
__printf_sd_place_emit:
        ld      a,c
        call    __stdio_emit_a
        ret
