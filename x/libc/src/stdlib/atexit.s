        ;; atexit.s
        ;; Split from exit_core.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module atexit
        .optsdcc -mz80 sdcccall(1)

        .globl  _atexit
        .globl  __libc_atexit_count
        .globl  __libc_atexit_handlers
        .globl  __libc_register_handler

        .area   _CODE
_atexit::
        ld      de,#__libc_atexit_handlers
        ld      bc,#__libc_atexit_count
        jp      __libc_register_handler

