        ;; wcstoimax.s
        ;;
        ;; intmax_t is long long on this target, so the wide-string wrapper can
        ;; tail-call wcstoll with no ABI translation.

        .module wcstoimax
        .optsdcc -mz80 sdcccall(1)

        .globl  _wcstoimax
        .globl  _wcstoll

        .area   _CODE

_wcstoimax::
        jp      _wcstoll
