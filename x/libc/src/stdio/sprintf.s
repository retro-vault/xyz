        ;; sprintf.s
        ;; Split from printf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module sprintf
        .optsdcc -mz80 sdcccall(0)

        .globl  _sprintf
        .globl  __stdio_alloc_ctx
        .globl  __stdio_init_string
        .globl  __stdio_store_ap_hl
        .globl  __stdio_store_fmt_hl
        .globl  __stdio_vformat

        .area   _CODE
_sprintf::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __stdio_alloc_ctx
        ld      l,4(ix)
        ld      h,5(ix)
        call    __stdio_init_string
        ld      l,6(ix)
        ld      h,7(ix)
        call    __stdio_store_fmt_hl
        push    ix
        pop     hl
        ld      de,#0x0008
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
