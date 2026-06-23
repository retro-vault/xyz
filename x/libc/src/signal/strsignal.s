        ;; strsignal.s
        ;;
        ;; POSIX strsignal() for the xcc Z80 libc.
        ;; The signal subsystem only exposes the six ISO C signals carried by
        ;; signal.h, so the string table is a direct 1..6 mapping with one
        ;; generic fallback for unknown numbers.

        .module strsignal
        .optsdcc -mz80 sdcccall(1)

        .globl  _strsignal

        .area   _CONST
__strsignal_unknown:
        .asciz  "Unknown signal"
__strsignal_abrt:
        .asciz  "Aborted"
__strsignal_fpe:
        .asciz  "Floating point exception"
__strsignal_ill:
        .asciz  "Illegal instruction"
__strsignal_int:
        .asciz  "Interrupt"
__strsignal_segv:
        .asciz  "Segmentation fault"
__strsignal_term:
        .asciz  "Terminated"
__strsignal_table:
        .dw     __strsignal_abrt
        .dw     __strsignal_fpe
        .dw     __strsignal_ill
        .dw     __strsignal_int
        .dw     __strsignal_segv
        .dw     __strsignal_term

        .area   _CODE

_strsignal::
        ld      a,h
        or      a
        jr      nz,__strsignal_fallback
        ld      a,l
        or      a
        jr      z,__strsignal_fallback
        cp      #7
        jr      nc,__strsignal_fallback

        dec     a
        add     a,a
        ld      c,a
        ld      b,#0
        ld      hl,#__strsignal_table
        add     hl,bc
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ret

__strsignal_fallback:
        ld      de,#__strsignal_unknown
        ret
