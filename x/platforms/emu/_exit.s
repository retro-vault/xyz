        ;; _exit.s  (sys backend: emu)
        ;;
        ;; void _exit(int status)
        ;;   HL = status (sdcccall(1))

        .module _exit
        .optsdcc -mz80 sdcccall(1)

        .globl  __exit

        .equ    EMU_RESULT,0xff00
        .equ    EMU_DONE,0xff02
        .equ    EMU_DONE_MAGIC,0xa5
        .equ    EMU_PORT_CMD,0xe0
        .equ    EMU_CMD_EXIT,1

        .area   _CODE
__exit::
        ld      (EMU_RESULT),hl
        ld      a,#EMU_CMD_EXIT
        out     (EMU_PORT_CMD),a
        ld      a,#EMU_DONE_MAGIC
        ld      (EMU_DONE),a
sys_exit_halt:
        halt
        jr      sys_exit_halt
