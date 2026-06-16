        ;; rand_state.s
        ;; Split from rand.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module rand_state
        .optsdcc -mz80 sdcccall(1)

        .globl  __rand_state

        .area   _DATA
__rand_state::
        .dw     1, 0                    ; initial state = 1 (as in C)
