        ;; wcstold.s
        ;;
        ;; long double shares double's ABI and representation on this target,
        ;; so wcstold can tail-call the shared wide double parser.

        .module wcstold
        .optsdcc -mz80 sdcccall(1)

        .globl  _wcstold
        .globl  __wcstod_core

        .area   _CODE

_wcstold::
        jp      __wcstod_core
