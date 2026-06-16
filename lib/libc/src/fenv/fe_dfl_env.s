        ;; fe_dfl_env.s
        ;; Split from fenv_common.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module fe_dfl_env
        .optsdcc -mz80 sdcccall(1)

        .globl  __fe_dfl_env

        .area   _CODE
__fe_dfl_env::
        .dw     0                       ; excepts
        .dw     0                       ; rounding (FE_TONEAREST)
