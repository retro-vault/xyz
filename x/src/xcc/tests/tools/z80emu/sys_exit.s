        ;; sys_exit.s
        ;;
        ;; Test-harness process exit hook. The executable runner watches the
        ;; same mailbox as the tiny crt0 stubs, so library-driven exit paths
        ;; need to publish the requested status there before halting.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module xcc_test_sys_exit
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_exit
        .globl  ___sys_exit
        .globl  __sys_exit_status
        .globl  ___sys_exit_status

        .area   _DATA
__sys_exit_status:
___sys_exit_status::
        .dw     0

        .area   _CODE

__sys_exit:
___sys_exit::
        ld      (__sys_exit_status),hl
        ld      (#0xff00),hl
        ld      a,#0xa5
        ld      (#0xff02),a
sys_exit_halt:
        halt
        jr      sys_exit_halt
