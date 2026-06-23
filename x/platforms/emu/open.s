        ;; open.s  (sys backend: emu)
        ;;
        ;; int open(const char *path, int flags, int mode)
        ;;   HL = path, DE = flags, BC = mode

        .module open
        .optsdcc -mz80 sdcccall(1)

        .globl  _open

        .equ    EMU_PORT_CMD,0xe0
        .equ    EMU_REQ_FD,0xff11
        .equ    EMU_REQ_PTR,0xff13
        .equ    EMU_REQ_LEN,0xff15
        .equ    EMU_REQ_FLAGS,0xff17
        .equ    EMU_REQ_MODE,0xff19
        .equ    EMU_REQ_PATH,0xff21
        .equ    EMU_REQ_RESULT,0xff25
        .equ    EMU_CMD_OPEN,4

        .area   _CODE
_open::
        ld      (EMU_REQ_PATH),hl
        ld      (EMU_REQ_FLAGS),de
        ld      (EMU_REQ_MODE),bc
        ld      a,#EMU_CMD_OPEN
        out     (EMU_PORT_CMD),a
        ld      de,(EMU_REQ_RESULT)
        ret
