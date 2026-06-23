        ;; setvbuf.s
        ;;
        ;; The current FILE layer is intentionally unbuffered. setvbuf() accepts
        ;; the standard mode constants for compatibility and reports success
        ;; after validating the mode value.

        .module setvbuf
        .optsdcc -mz80 sdcccall(1)

        .globl  _setvbuf

        .area   _CODE

_setvbuf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      a,5(ix)
        or      a
        jr      nz,setvbuf_fail
        ld      a,4(ix)
        cp      #0
        jr      z,setvbuf_ok
        cp      #1
        jr      z,setvbuf_ok
        cp      #2
        jr      z,setvbuf_ok
setvbuf_fail:
        ld      hl,#0xffff
        push    hl
        pop     de
        pop     ix
        ret
setvbuf_ok:
        ld      hl,#0x0000
        push    hl
        pop     de
        pop     ix
        ret
