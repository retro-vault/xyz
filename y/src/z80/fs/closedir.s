        ; Close and release a YOS directory object.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module closedir
        .optsdcc -mz80 sdcccall(1)

        .globl  _closedir
        .globl  __directory_validate
        .globl  __yos_free
        .globl  __zx_esx_errno
        .globl  __zx_esx_error
        .globl  __zx_esx_f_close

        .area   _CODE

        ; input: HL = DIR pointer; output: DE = 0 or -1.
_closedir::
        call    __directory_validate
        jp      c,__zx_esx_errno
        push    hl
        call    __zx_esx_f_close
        pop     hl
        jp      c,__zx_esx_error
        push    hl
        inc     hl
        ld      (hl),#0
        pop     hl
        call    __yos_free
        ld      de,#0
        ret
