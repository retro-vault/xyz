        ;; tmpnam.s
        ;;
        ;; Generate a deterministic temporary-file name in either the caller's
        ;; buffer or a private static buffer. The name generator is simple but
        ;; stable: "tmpXXXX.tmp" with a monotonically increasing hex suffix.

        .module tmpnam
        .optsdcc -mz80 sdcccall(1)

        .globl  _tmpnam

        .area   _DATA
__stdio_tmpnam_counter:
        .dw     0
__stdio_tmpnam_buf:
        .ds     12

        .area   _CONST
__stdio_tmpnam_hex:
        .ascii  "0123456789abcdef"

        .area   _CODE

__stdio_tmpnam_emit_nibble:
        and     #0x0f
        push    de
        ld      e,a
        ld      d,#0x00
        push    hl
        ld      hl,#__stdio_tmpnam_hex
        add     hl,de
        ld      a,(hl)
        pop     hl
        ld      (hl),a
        inc     hl
        pop     de
        ret

_tmpnam::
        ld      a,h
        or      l
        jr      nz,tmpnam_have_buf
        ld      hl,#__stdio_tmpnam_buf
tmpnam_have_buf:
        push    hl
        ld      (hl),#'t'
        inc     hl
        ld      (hl),#'m'
        inc     hl
        ld      (hl),#'p'
        inc     hl
        ld      de,(__stdio_tmpnam_counter)
        ld      a,d
        and     #0xf0
        rrca
        rrca
        rrca
        rrca
        call    __stdio_tmpnam_emit_nibble
        ld      a,d
        call    __stdio_tmpnam_emit_nibble
        ld      a,e
        and     #0xf0
        rrca
        rrca
        rrca
        rrca
        call    __stdio_tmpnam_emit_nibble
        ld      a,e
        call    __stdio_tmpnam_emit_nibble
        ld      (hl),#'.'
        inc     hl
        ld      (hl),#'t'
        inc     hl
        ld      (hl),#'m'
        inc     hl
        ld      (hl),#'p'
        inc     hl
        xor     a
        ld      (hl),a
        ld      hl,(__stdio_tmpnam_counter)
        inc     hl
        ld      (__stdio_tmpnam_counter),hl
        pop     hl
        push    hl
        pop     de
        ret
