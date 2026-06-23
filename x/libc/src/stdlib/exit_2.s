        ;; exit_2.s
        ;; Split from exit_core.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module exit_2
        .optsdcc -mz80 sdcccall(1)

        .globl  __Exit
        .globl  __exit
        .globl  __libc_exit_kind
        .globl  __libc_exit_status

        .area   _CODE
__Exit::
        ld      (__libc_exit_status),hl
        ld      hl,#3
        ld      (__libc_exit_kind),hl
        ld      hl,(__libc_exit_status)
        call    __exit
_Exit_halt:
        jr      _Exit_halt

