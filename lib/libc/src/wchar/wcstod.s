        ;; wcstod.s
        ;;
        ;; Public wide-string double parser wrapper around the shared
        ;; transcode-and-parse core.

        .module wcstod
        .optsdcc -mz80 sdcccall(1)

        .globl  _wcstod
        .globl  __wcstod_core

        .area   _CODE

_wcstod::
        jp      __wcstod_core
