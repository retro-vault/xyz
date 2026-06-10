        ;; wcstoumax.s
        ;;
        ;; uintmax_t is unsigned long long on this target, so wcstoumax shares
        ;; wcstoull's exact calling convention and return layout.

        .module wcstoumax
        .optsdcc -mz80 sdcccall(1)

        .globl  _wcstoumax
        .globl  _wcstoull

        .area   _CODE

_wcstoumax::
        jp      _wcstoull
