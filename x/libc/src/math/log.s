        ;; log.s
        ;; Split from legacymathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module log
        .optsdcc -mz80 sdcccall(1)

        .globl  _log
        .globl  _logl
        .globl  _logd_core

        .area   _CODE
_log::
_logl::
        jp      _logd_core
