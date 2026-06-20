        ;; read.s  (sys backend: emu)
        ;;
        ;; int read(int fd, void *buf, unsigned len)
        ;;   HL = fd, DE = buf, len at IX+4 after prologue

        .module read
        .optsdcc -mz80 sdcccall(1)

        .globl  _read

        .equ    EMU_PORT_CMD,0xe0
        .equ    EMU_REQ_FD,0xff11
        .equ    EMU_REQ_PTR,0xff13
        .equ    EMU_REQ_LEN,0xff15
        .equ    EMU_REQ_RESULT,0xff25
        .equ    EMU_CMD_READ,6

        .area   _CODE
_read::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      (EMU_REQ_FD),hl
        ld      (EMU_REQ_PTR),de
        ld      c,4(ix)
        ld      b,5(ix)
        ld      (EMU_REQ_LEN),bc
        ld      a,#EMU_CMD_READ
        out     (EMU_PORT_CMD),a
        ld      de,(EMU_REQ_RESULT)
        pop     ix
        ret
