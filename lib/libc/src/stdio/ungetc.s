        ;; ungetc.s
        ;; Split from stdio_io.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module ungetc
        .optsdcc -mz80 sdcccall(1)

        .globl  _ungetc
        .globl  __stdio_io_clear_flags
        .globl  __stdio_io_require_stream

FILE_OFF_PUSHV  .equ 2

        .area   _CODE
_ungetc::
        push    hl
        ex      de,hl
        call    __stdio_io_require_stream
        ld      a,h
        cp      #0xff
        jr      z,__stdio_io_ungetc_fail_popchar
        push    hl
        ld      de,#FILE_OFF_PUSHV
        add     hl,de
        ld      a,(hl)
        pop     hl
        or      a
        jr      nz,__stdio_io_ungetc_fail_popchar
        pop     de
        ld      a,e
        cp      #0xff
        jr      z,__stdio_io_ungetc_fail
        push    de
        call    __stdio_io_clear_flags
        pop     de
        ld      bc,#FILE_OFF_PUSHV
        add     hl,bc
        ld      a,#1
        ld      (hl),a
        inc     hl
        ld      a,e
        ld      (hl),a
        ld      l,a
        ld      h,#0x00
        push    hl
        pop     de
        ret
__stdio_io_ungetc_fail_popchar:
        pop     bc
__stdio_io_ungetc_fail:
        ld      hl,#0xffff
        push    hl
        pop     de
        ret

