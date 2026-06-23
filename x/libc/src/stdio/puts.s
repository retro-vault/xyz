        ;; puts.s
        ;; Split from printf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module puts
        .optsdcc -mz80 sdcccall(1)

        .globl  _puts
        .globl  __stdio_alloc_ctx
        .globl  __stdio_emit_a
        .globl  __stdio_emit_string_field
        .globl  __stdio_init_console
        .globl  __stdio_reset_field_state

        .area   _CODE
_puts::
        push    hl
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __stdio_alloc_ctx
        call    __stdio_init_console
        call    __stdio_reset_field_state
        ld      l,2(ix)
        ld      h,3(ix)
        push    ix
        call    __stdio_emit_string_field
        pop     ix
        ld      a,#'\n'
        push    ix
        call    __stdio_emit_a
        pop     ix
        ld      sp,ix
        pop     ix
        pop     hl
        ld      hl,#0x0001
        push    hl
        pop     de
        ret

