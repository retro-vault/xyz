        ;; at_quick_exit.s
        ;; Split from exit_core.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module at_quick_exit
        .optsdcc -mz80 sdcccall(1)

        .globl  _at_quick_exit
        .globl  __libc_quick_exit_count
        .globl  __libc_quick_exit_handlers
        .globl  __libc_register_handler

        .area   _CODE
_at_quick_exit::
        ld      de,#__libc_quick_exit_handlers
        ld      bc,#__libc_quick_exit_count
        jp      __libc_register_handler

