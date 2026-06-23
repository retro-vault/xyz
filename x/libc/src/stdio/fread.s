        ;; fread.s
        ;; Split from stdio_io.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module fread
        .optsdcc -mz80 sdcccall(1)

        .globl  _fread
        .globl  __stdio_io_getc_core
        .globl  __stdio_io_require_stream
        .globl  __stdio_io_rw_zero

        .area   _CODE
_fread::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    hl
        ld      hl,#0x0000
        push    hl
        push    de
        ld      c,4(ix)
        ld      b,5(ix)
        push    bc
        ld      hl,#0x0000
        push    hl
        ld      l,6(ix)
        ld      h,7(ix)
        call    __stdio_io_require_stream
        ld      a,h
        cp      #0xff
        jp      z,__stdio_io_rw_zero
        ld      -4(ix),l
        ld      -3(ix),h
__stdio_io_fread_item:
        ld      c,-8(ix)
        ld      b,-7(ix)
        ld      a,b
        or      c
        jr      z,__stdio_io_fread_done
        ld      c,-6(ix)
        ld      b,-5(ix)
__stdio_io_fread_byte:
        ld      a,b
        or      c
        jr      z,__stdio_io_fread_item_done
        push    bc
        ld      l,-4(ix)
        ld      h,-3(ix)
        call    __stdio_io_getc_core
        pop     bc
        ld      a,h
        cp      #0xff
        jr      z,__stdio_io_fread_abort
        ld      e,-2(ix)
        ld      d,-1(ix)
        ld      a,l
        ld      (de),a
        inc     de
        ld      -2(ix),e
        ld      -1(ix),d
        dec     bc
        jr      __stdio_io_fread_byte
__stdio_io_fread_item_done:
        ld      l,-8(ix)
        ld      h,-7(ix)
        dec     hl
        ld      -8(ix),l
        ld      -7(ix),h
        ld      l,-10(ix)
        ld      h,-9(ix)
        inc     hl
        ld      -10(ix),l
        ld      -9(ix),h
        jr      __stdio_io_fread_item
__stdio_io_fread_abort:
__stdio_io_fread_done:
        ld      l,-10(ix)
        ld      h,-9(ix)
        push    hl
        pop     de
        ld      sp,ix
        pop     ix
        ret

