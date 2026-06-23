        ;; rename.s  (sys backend: emu)

        .module rename
        .optsdcc -mz80 sdcccall(1)

        .globl  _rename

        .equ    EMU_PORT_CMD,0xe0
        .equ    EMU_REQ_PATH,0xff21
        .equ    EMU_REQ_PATH2,0xff23
        .equ    EMU_REQ_RESULT,0xff25
        .equ    EMU_CMD_RENAME,10

        .area   _CODE
_rename::
        ld      (EMU_REQ_PATH),hl
        ld      (EMU_REQ_PATH2),de
        ld      a,#EMU_CMD_RENAME
        out     (EMU_PORT_CMD),a
        ld      de,(EMU_REQ_RESULT)
        ret
