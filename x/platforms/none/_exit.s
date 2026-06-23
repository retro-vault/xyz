        ;; _exit.s  (sys backend: none — template)
        ;;
        ;; void _exit(int status)          (backs exit()/_Exit())
        ;;   HL = status                        (sdcccall(1))
        ;;   does not return.
        ;;
        ;; With no operating system to return to, halt the CPU.  On a hosted
        ;; target, hand `status` back to the supervisor / monitor instead.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module _exit
        .optsdcc -mz80 sdcccall(1)

        .globl  __exit

        .area   _CODE
__exit::
        ;; TODO: report `status` (in HL) to your supervisor if there is one.
        di
sys_exit_halt:
        halt
        jr      sys_exit_halt
