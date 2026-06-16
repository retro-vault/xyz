        ;; abort.s
        ;; Split from exit_core.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module abort
        .optsdcc -mz80 sdcccall(1)

        .globl  _abort
        .globl  __exit
        .globl  __libc_exit_kind
        .globl  __libc_exit_status

        .area   _CODE
_abort::
        ld      hl,#2
        ld      (__libc_exit_kind),hl
        ld      hl,#1
        ld      (__libc_exit_status),hl
        call    __exit
abort_halt:
        jr      abort_halt

