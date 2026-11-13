        ; Close an native filesystem file descriptor.
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module close
        .optsdcc -mz80 sdcccall(1)

        .globl  _close
        .globl  __critical_call
        .globl  __next_fs_fd
        .globl  __next_fs_errno
        .globl  __next_fs_error
        .globl  __next_fs_f_close

        .area   _CODE

        ; inputs: HL = fd; output: DE = 0 or -1.
        ; clobbers: af, bc, de, hl. IX/IY preserved.
_close::
        call    __critical_call
        call    __next_fs_fd
        jp      c,__next_fs_errno
        push    hl
        call    __next_fs_f_close
        pop     hl
        jp      c,__next_fs_error
        inc     hl
        ld      (hl),#0
        ld      de,#0
        ret
