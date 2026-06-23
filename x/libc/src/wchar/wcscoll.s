        ;; wcscoll.s
        ;;
        ;; The current libc is locale-neutral, so wide collation is the same
        ;; lexicographic ordering as wcscmp().

        .module wcscoll
        .optsdcc -mz80 sdcccall(1)

        .globl  _wcscoll
        .globl  _wcscmp

        .area   _CODE

_wcscoll::
        jp      _wcscmp
