        ;; getwc.s
        ;;
        ;; C defines getwc as the stream-parameter form of fgetwc. The stack
        ;; layout and return convention are identical, so this is a straight
        ;; tail-call alias.

        .module getwc
        .optsdcc -mz80 sdcccall(1)

        .globl  _getwc
        .globl  _fgetwc

        .area   _CODE

_getwc::
        jp      _fgetwc
