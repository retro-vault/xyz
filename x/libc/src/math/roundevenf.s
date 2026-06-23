        ;; roundevenf.s
        ;; Split from moremathf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module roundevenf
        .optsdcc -mz80 sdcccall(1)

        .globl  _roundevenf
        .globl  _roundf

        .area   _CODE
_roundevenf::
        ; for basic, alias to round (full would adjust tie to even using bits)
        jp      _roundf

