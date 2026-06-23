        ;; wcstof.s
        ;;
        ;; Public wide-string float parser wrapper. The shared wide core parses
        ;; through the double runtime first, then this wrapper narrows the
        ;; result to float32.

        .module wcstof
        .optsdcc -mz80 sdcccall(1)

        .globl  _wcstof
        .globl  __wcstod_core
        .globl  ___db2fs

        .area   _CODE

_wcstof::
        call    __wcstod_core
        jp      ___db2fs
