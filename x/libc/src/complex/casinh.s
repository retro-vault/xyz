        ;; casinh.s
        ;; Split from casinhf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module casinh
        .optsdcc -mz80 sdcccall(1)

        .globl  _casinh
        .globl  _casinhl
        .globl  _casinhf

        .area   _CODE
_casinh::
_casinhl::
        jp      _casinhf
