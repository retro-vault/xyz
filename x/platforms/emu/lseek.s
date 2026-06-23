        ;; lseek.s  (sys backend: emu)
        ;;
        ;; long lseek(int fd, long offset, int whence)
        ;; The current ABI places the 32-bit offset at IX+4..IX+7 and whence
        ;; at IX+8..IX+9 after the usual frame setup.

        .module lseek
        .optsdcc -mz80 sdcccall(1)

        .globl  _lseek

        .equ    EMU_PORT_CMD,0xe0
        .equ    EMU_REQ_FD,0xff11
        .equ    EMU_REQ_WHENCE,0xff1b
        .equ    EMU_REQ_OFFSET,0xff1d
        .equ    EMU_REQ_RESULT,0xff25
        .equ    EMU_CMD_LSEEK,8

        .area   _CODE
_lseek::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      (EMU_REQ_FD),hl
        ld      a,4(ix)
        ld      (EMU_REQ_OFFSET),a
        ld      a,5(ix)
        ld      (EMU_REQ_OFFSET + 1),a
        ld      a,6(ix)
        ld      (EMU_REQ_OFFSET + 2),a
        ld      a,7(ix)
        ld      (EMU_REQ_OFFSET + 3),a
        ld      a,8(ix)
        ld      (EMU_REQ_WHENCE),a
        ld      a,9(ix)
        ld      (EMU_REQ_WHENCE + 1),a
        ld      a,#EMU_CMD_LSEEK
        out     (EMU_PORT_CMD),a
        ld      de,(EMU_REQ_RESULT)
        ld      hl,(EMU_REQ_RESULT + 2)
        pop     ix
        ret
