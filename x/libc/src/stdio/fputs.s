        ;; fputs.s
        ;; Split from printf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module fputs
        .optsdcc -mz80 sdcccall(1)

        .globl  _fputs
        .globl  __stdio_alloc_ctx
        .globl  __stdio_emit_string_field
        .globl  __stdio_init_console_fd
        .globl  __stdio_reset_field_state
        .globl  __stdio_stream_accepts_output

        .area   _CODE
_fputs::
        push    de
        push    hl
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __stdio_alloc_ctx
        ld      l,4(ix)
        ld      h,5(ix)
        call    __stdio_stream_accepts_output
        ld      a,h
        cp      #0xFF
        jr      nz,__stdio_fputs_ok
        ld      sp,ix
        pop     ix
        pop     hl
        pop     de
        ld      hl,#0xFFFF
        push    hl
        pop     de
        ret
__stdio_fputs_ok:
        ld      a,(hl)
        call    __stdio_init_console_fd
        call    __stdio_reset_field_state
        ld      l,2(ix)
        ld      h,3(ix)
        push    ix
        call    __stdio_emit_string_field
        pop     ix
        ld      sp,ix
        pop     ix
        pop     hl
        pop     de
        ld      hl,#0x0001
        push    hl
        pop     de
        ret

