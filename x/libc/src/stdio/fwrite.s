        ;; fwrite.s
        ;; Split from stdio_io.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module fwrite
        .optsdcc -mz80 sdcccall(1)

        .globl  _fwrite
        .globl  __stdio_io_clear_flags
        .globl  __stdio_io_require_stream
        .globl  __stdio_io_rw_zero
        .globl  __stdio_emit_bytes

FILE_FLAG_ERR   .equ 0x02

        .area   _CODE
__stdio_io_putc_core:
        call    __stdio_io_require_stream
        ld      a,h
        cp      #0xff
        ret     z
        push    de
        call    __stdio_io_clear_flags
        pop     de
        ld      b,h
        ld      c,l
        ld      d,#0x00
        push    bc
        push    de
        ld      hl,#0x0000
        add     hl,sp
        ex      de,hl
        ld      a,(bc)
        ld      l,a
        ld      h,#0x00
        ld      bc,#0x0001
        push    bc
        call    __stdio_emit_bytes
        pop     bc
        pop     hl
        pop     bc
        ld      a,d
        cp      #0xff
        jr      nz,__stdio_io_putc_count
        ld      a,e
        cp      #0xff
        jr      z,__stdio_io_putc_err
__stdio_io_putc_count:
        ld      a,d
        or      e
        jr      z,__stdio_io_putc_err
        ld      h,#0x00
        ret
__stdio_io_putc_err:
        ld      h,b
        ld      l,c
        inc     hl
        ld      a,(hl)
        or      #FILE_FLAG_ERR
        ld      (hl),a
        ld      hl,#0xffff
        ret

_fwrite::
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
__stdio_io_fwrite_item:
        ld      c,-8(ix)
        ld      b,-7(ix)
        ld      a,b
        or      c
        jr      z,__stdio_io_fwrite_done
        ld      c,-6(ix)
        ld      b,-5(ix)
__stdio_io_fwrite_byte:
        ld      a,b
        or      c
        jr      z,__stdio_io_fwrite_item_done
        push    bc
        ld      l,-2(ix)
        ld      h,-1(ix)
        ld      e,(hl)
        inc     hl
        ld      -2(ix),l
        ld      -1(ix),h
        ld      l,-4(ix)
        ld      h,-3(ix)
        call    __stdio_io_putc_core
        pop     bc
        ld      a,h
        cp      #0xff
        jr      z,__stdio_io_fwrite_abort
        dec     bc
        jr      __stdio_io_fwrite_byte
__stdio_io_fwrite_item_done:
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
        jr      __stdio_io_fwrite_item
__stdio_io_fwrite_abort:
__stdio_io_fwrite_done:
        ld      l,-10(ix)
        ld      h,-9(ix)
        push    hl
        pop     de
        ld      sp,ix
        pop     ix
        ret

