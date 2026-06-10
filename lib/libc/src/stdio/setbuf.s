        ;; setbuf.s
        ;;
        ;; Unbuffered stdio ignores caller-supplied buffers. The function is
        ;; still provided so hosted code can disable buffering portably.

        .module setbuf
        .optsdcc -mz80 sdcccall(1)

        .globl  _setbuf

        .area   _CODE

_setbuf::
        ret
