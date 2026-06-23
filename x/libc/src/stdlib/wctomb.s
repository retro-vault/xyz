        ;; wctomb.s
        ;;
        ;; Convert one wchar_t back into the execution charset. Only byte-range
        ;; wide characters are representable; wider code points report EILSEQ.

        .module wctomb
        .optsdcc -mz80 sdcccall(1)

        .globl  _wctomb
        .globl  __errno_value

        .area   _CODE

_wctomb::
        ld      a,h
        or      l
        jr      z,wctomb_zero
        ld      a,d
        or      a
        jr      nz,wctomb_bad
        ld      a,e
        ld      (hl),a
        ld      de,#0x0001
        ret
wctomb_zero:
        ld      de,#0x0000
        ret
wctomb_bad:
        ld      hl,#84
        ld      (__errno_value),hl
        ld      de,#0xffff
        ret
