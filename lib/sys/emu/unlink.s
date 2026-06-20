        ;; unlink.s  (sys backend: emu)

        .module unlink
        .optsdcc -mz80 sdcccall(1)

        .globl  _unlink

        .equ    EMU_PORT_CMD,0xe0
        .equ    EMU_REQ_PATH,0xff21
        .equ    EMU_REQ_RESULT,0xff25
        .equ    EMU_CMD_UNLINK,9

        .area   _CODE
_unlink::
        ld      (EMU_REQ_PATH),hl
        ld      a,#EMU_CMD_UNLINK
        out     (EMU_PORT_CMD),a
        ld      de,(EMU_REQ_RESULT)
        ret
