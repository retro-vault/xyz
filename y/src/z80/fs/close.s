        ; Close an esxDOS file descriptor.
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module close
        .optsdcc -mz80 sdcccall(1)

        .globl  _close
        .globl  __critical_call
        .globl  __zx_esx_fd
        .globl  __zx_esx_errno
        .globl  __zx_esx_error
        .globl  __zx_esx_f_close

        .area   _CODE

        ; inputs: HL = fd; output: DE = 0 or -1.
        ; clobbers: af, bc, de, hl. IX/IY preserved.
_close::
        call    __critical_call
        call    __zx_esx_fd
        jp      c,__zx_esx_errno
        push    hl
        call    __zx_esx_f_close
        pop     hl
        jp      c,__zx_esx_error
        inc     hl
        ld      (hl),#0
        ld      de,#0
        ret
