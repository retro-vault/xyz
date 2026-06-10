        ;; wcrtomb.s
        ;;
        ;; The target uses a stateless single-byte execution charset. That
        ;; makes wcrtomb() equivalent to wctomb() for all representable
        ;; characters, with the trailing mbstate_t parameter ignored.

        .module wcrtomb
        .optsdcc -mz80 sdcccall(1)

        .globl  _wcrtomb
        .globl  _wctomb

        .area   _CODE

_wcrtomb::
        jp      _wctomb
