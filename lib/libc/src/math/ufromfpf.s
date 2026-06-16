        ;; ufromfpf.s
        ;; Split from moremathf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module ufromfpf
        .optsdcc -mz80 sdcccall(1)

        .globl  _ufromfpf
        .globl  _ufromfpxf
        .globl  _roundf

        .area   _CODE
_ufromfpf::
_ufromfpxf::
        ; unsigned round
        call    _roundf
        ; clamp negative to 0
        bit     7,h
        ret     z
        ld      de,#0
        ld      hl,#0
        ret

