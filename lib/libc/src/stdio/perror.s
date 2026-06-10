        ;; perror.s
        ;;
        ;; Emit "prefix: message\\n" to file descriptor 2. The current libc only
        ;; knows the three standard C errno values it defines itself and falls
        ;; back to a generic "error" string for everything else.

        .module perror
        .optsdcc -mz80 sdcccall(1)

        .globl  _perror
        .globl  _write
        .globl  __errno_value

        .area   _CONST
__perror_sep:
        .asciz  ": "
__perror_nl:
        .asciz  "\n"
__perror_dom:
        .asciz  "domain error"
__perror_range:
        .asciz  "range error"
__perror_ilseq:
        .asciz  "illegal byte sequence"
__perror_generic:
        .asciz  "error"

        .area   _CODE

__perror_write_hl:
        push    hl
        ld      bc,#0x0000
__perror_len_loop:
        ld      a,(hl)
        or      a
        jr      z,__perror_len_done
        inc     hl
        inc     bc
        jr      __perror_len_loop
__perror_len_done:
        pop     de
        ld      a,b
        or      c
        ret     z
        push    bc
        ld      hl,#0x0002
        call    _write
        pop     bc
        ret

_perror::
        ld      a,h
        or      l
        jr      z,__perror_no_prefix
        ld      a,(hl)
        or      a
        jr      z,__perror_no_prefix
        call    __perror_write_hl
        ld      hl,#__perror_sep
        call    __perror_write_hl
__perror_no_prefix:
        ld      hl,(__errno_value)
        ld      a,l
        cp      #33
        jr      z,__perror_msg_dom
        cp      #34
        jr      z,__perror_msg_range
        cp      #84
        jr      z,__perror_msg_ilseq
        ld      hl,#__perror_generic
        jr      __perror_emit_msg
__perror_msg_dom:
        ld      hl,#__perror_dom
        jr      __perror_emit_msg
__perror_msg_range:
        ld      hl,#__perror_range
        jr      __perror_emit_msg
__perror_msg_ilseq:
        ld      hl,#__perror_ilseq
__perror_emit_msg:
        call    __perror_write_hl
        ld      hl,#__perror_nl
        call    __perror_write_hl
        ret
