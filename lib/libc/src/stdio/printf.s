        ;; printf.s
        ;;
        ;; Minimal output-oriented stdio core for the xcc Z80 libc.
        ;; All public entry points use sdcccall(0), so every fixed and variadic
        ;; argument lives on the stack in a simple linear layout.
        ;;
        ;; Supported conversions:
        ;;   %d %i %u %x %X %o %c %s %p %n %%
        ;;
        ;; Supported flags / fields:
        ;;   - + space # 0
        ;;   width, precision, *, h, hh, l, ll, j, z, t
        ;;
        ;; Floating-point formatting is intentionally left for a later pass.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih








        .module printf
        .optsdcc -mz80 sdcccall(1)

        .globl  _printf
        .globl  __stdio_alloc_ctx
        .globl  __stdio_init_console
        .globl  __stdio_store_ap_hl
        .globl  __stdio_store_fmt_hl
        .globl  __stdio_vformat

        .area   _CODE
_printf::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __stdio_alloc_ctx
        call    __stdio_init_console
        ld      l,4(ix)
        ld      h,5(ix)
        call    __stdio_store_fmt_hl
        push    ix
        pop     hl
        ld      de,#0x0006
        add     hl,de
        call    __stdio_store_ap_hl
        push    ix
        call    __stdio_vformat
        pop     ix
        push    hl
        pop     de
        ld      sp,ix
        pop     ix
        ret

