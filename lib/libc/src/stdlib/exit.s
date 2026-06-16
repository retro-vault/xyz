        ;; exit.s
        ;; Split from exit_core.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module exit
        .optsdcc -mz80 sdcccall(1)

        .globl  _exit
        .globl  __exit
        .globl  __libc_atexit_count
        .globl  __libc_atexit_handlers
        .globl  __libc_exit_kind
        .globl  __libc_exit_status
        .globl  __libc_run_handlers

        .area   _CODE
_exit::
        push    hl
        ld      hl,#__libc_atexit_handlers
        ld      de,(__libc_atexit_count)
        call    __libc_run_handlers
        pop     hl
        ld      (__libc_exit_status),hl
        ld      hl,#1
        ld      (__libc_exit_kind),hl
        ld      hl,(__libc_exit_status)
        call    __exit
exit_halt:
        jr      exit_halt

