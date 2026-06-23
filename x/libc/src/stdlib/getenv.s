        ;; getenv.s
        ;;
        ;; This target currently has no hosted process environment. getenv()
        ;; therefore always reports "not found".

        .module getenv
        .optsdcc -mz80 sdcccall(1)

        .globl  _getenv

        .area   _CODE

_getenv::
        ld      de,#0x0000
        ret
