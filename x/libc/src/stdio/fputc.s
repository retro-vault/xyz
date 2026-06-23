        ;; fputc.s
        ;; Split from printf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module fputc
        .optsdcc -mz80 sdcccall(1)

        .globl  _fputc
        .globl  __stdio_alloc_ctx
        .globl  __stdio_emit_a
        .globl  __stdio_init_console_fd
        .globl  __stdio_stream_accepts_output

        .area   _CODE
_fputc::
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
        jr      nz,__stdio_fputc_ok
        ld      sp,ix
        pop     ix
        pop     hl
        pop     de
        ld      hl,#0xFFFF
        push    hl
        pop     de
        ret
__stdio_fputc_ok:
        ld      a,(hl)
        call    __stdio_init_console_fd
        ld      a,2(ix)
        push    ix
        call    __stdio_emit_a
        pop     ix
        ld      sp,ix
        pop     ix
        pop     hl
        pop     de
        ld      h,#0x00
        push    hl
        pop     de
        ret

