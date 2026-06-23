        ;; nearbyintf.s
        ;; Split from moremathf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module nearbyintf
        .optsdcc -mz80 sdcccall(1)

        .globl  _nearbyintf
        .globl  _rintf

        .area   _CODE
_nearbyintf::
        jp      _rintf

