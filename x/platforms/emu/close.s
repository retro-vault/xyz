        ;; close.s  (sys backend: emu)

        .module close
        .optsdcc -mz80 sdcccall(1)

        .globl  _close

        .equ    EMU_PORT_CMD,0xe0
        .equ    EMU_REQ_FD,0xff11
        .equ    EMU_REQ_RESULT,0xff25
        .equ    EMU_CMD_CLOSE,5

        .area   _CODE
_close::
        ld      (EMU_REQ_FD),hl
        ld      a,#EMU_CMD_CLOSE
        out     (EMU_PORT_CMD),a
        ld      de,(EMU_REQ_RESULT)
        ret
