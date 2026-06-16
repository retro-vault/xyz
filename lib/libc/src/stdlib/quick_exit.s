        ;; quick_exit.s
        ;; Split from exit_core.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module quick_exit
        .optsdcc -mz80 sdcccall(1)

        .globl  _quick_exit
        .globl  __exit
        .globl  __libc_exit_kind
        .globl  __libc_exit_status
        .globl  __libc_quick_exit_count
        .globl  __libc_quick_exit_handlers
        .globl  __libc_run_handlers

        .area   _CODE
_quick_exit::
        push    hl
        ld      hl,#__libc_quick_exit_handlers
        ld      de,(__libc_quick_exit_count)
        call    __libc_run_handlers
        pop     hl
        ld      (__libc_exit_status),hl
        ld      hl,#4
        ld      (__libc_exit_kind),hl
        ld      hl,(__libc_exit_status)
        call    __exit
quick_exit_halt:
        jr      quick_exit_halt
