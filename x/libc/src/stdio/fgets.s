        ;; fgets.s
        ;; Split from stdio_io.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module fgets
        .optsdcc -mz80 sdcccall(1)

        .globl  _fgets
        .globl  __stdio_io_getc_core
        .globl  __stdio_io_require_stream

        .area   _CODE
_fgets::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    hl
        push    hl
        ld      hl,#0x0000
        push    hl
        push    hl
        ld      l,-2(ix)
        ld      h,-1(ix)
        ld      a,h
        or      l
        jp      z,__stdio_io_fgets_fail_s
        ld      c,e
        ld      b,d
        ld      a,b
        or      c
        jp      z,__stdio_io_fgets_fail_n
        ld      l,4(ix)
        ld      h,5(ix)
        call    __stdio_io_require_stream
        ld      a,h
        cp      #0xff
        jp      z,__stdio_io_fgets_fail_stream
        ld      -8(ix),l
        ld      -7(ix),h
        dec     bc
        jp      z,__stdio_io_fgets_empty
__stdio_io_fgets_loop:
        push    bc
        ld      l,-8(ix)
        ld      h,-7(ix)
        call    __stdio_io_getc_core
        pop     bc
        ld      a,h
        cp      #0xff
        jp      z,__stdio_io_fgets_eof
        ld      e,-4(ix)
        ld      d,-3(ix)
        ld      a,l
        ld      (de),a
        inc     de
        ld      -4(ix),e
        ld      -3(ix),d
        ld      l,-6(ix)
        ld      h,-5(ix)
        inc     hl
        ld      -6(ix),l
        ld      -5(ix),h
        cp      #'\n'
        jr      z,__stdio_io_fgets_done_store
        dec     bc
        ld      a,b
        or      c
        jr      nz,__stdio_io_fgets_loop
__stdio_io_fgets_done_store:
        xor     a
        ld      e,-4(ix)
        ld      d,-3(ix)
        ld      (de),a
        ld      a,-6(ix)
        ld      d,a
        ld      a,-5(ix)
        ld      e,a
        ld      a,d
        or      e
        jr      nz,__stdio_io_fgets_return_ptr
        ld      hl,#0x0000
        ld      sp,ix
        pop     ix
        ret
__stdio_io_fgets_return_ptr:
        ld      l,-2(ix)
        ld      h,-1(ix)
        push    hl
        pop     de
        ld      sp,ix
        pop     ix
        ret
__stdio_io_fgets_eof:
        ld      l,-6(ix)
        ld      h,-5(ix)
        ld      a,h
        or      l
        jr      nz,__stdio_io_fgets_done_store
__stdio_io_fgets_fail_s:
__stdio_io_fgets_fail_n:
__stdio_io_fgets_fail_stream:
__stdio_io_fgets_fail:
        ld      hl,#0x0000
        push    hl
        pop     de
        ld      sp,ix
        pop     ix
        ret
__stdio_io_fgets_empty:
        xor     a
        ld      l,-2(ix)
        ld      h,-1(ix)
        ld      (hl),a
        ld      l,-2(ix)
        ld      h,-1(ix)
        push    hl
        pop     de
        ld      sp,ix
        pop     ix
        ret

        ;; Nested byte loop: return number of complete items moved in HL.
