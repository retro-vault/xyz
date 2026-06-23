        ;; fprintf.s
        ;; Split from printf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module fprintf
        .optsdcc -mz80 sdcccall(1)

        .globl  _fprintf
        .globl  __stdio_alloc_ctx
        .globl  __stdio_init_console_fd
        .globl  __stdio_store_ap_hl
        .globl  __stdio_store_fmt_hl
        .globl  __stdio_stream_accepts_output
        .globl  __stdio_vformat

        .area   _CODE
_fprintf::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __stdio_alloc_ctx
        ld      l,4(ix)
        ld      h,5(ix)
        call    __stdio_stream_accepts_output
        ld      a,h
        cp      #0xFF
        jr      nz,__stdio_fprintf_ok
        ld      hl,#0xFFFF
        ld      sp,ix
        pop     ix
        push    hl
        pop     de
        ret
__stdio_fprintf_ok:
        ld      a,(hl)
        call    __stdio_init_console_fd
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

