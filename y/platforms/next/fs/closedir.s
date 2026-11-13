        ; Close and release a YOS directory object.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module closedir
        .optsdcc -mz80 sdcccall(1)

        .globl  _closedir
        .globl  __critical_call
        .globl  __directory_validate
        .globl  __near_free
        .globl  __next_fs_errno
        .globl  __next_fs_error
        .globl  __next_fs_f_close

        .area   _CODE

        ; input: HL = DIR pointer; output: DE = 0 or -1.
_closedir::
        call    __critical_call
        call    __directory_validate
        jp      c,__next_fs_errno
        push    hl
        call    __next_fs_f_close
        pop     hl
        jp      c,__next_fs_error
        push    hl
        inc     hl
        ld      (hl),#0
        pop     hl
        call    __near_free
        ld      de,#0
        ret
