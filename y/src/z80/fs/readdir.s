        ; Read and convert the next entry from a YOS directory.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module readdir
        .optsdcc -mz80 sdcccall(1)

        .globl  _readdir
        .globl  __directory_validate
        .globl  __directory_convert
        .globl  __zx_esx_errno
        .globl  __zx_esx_error
        .globl  __zx_esx_f_readdir

        .area   _CODE

        ; input: HL = DIR pointer; output: DE = dirent pointer or NULL.
_readdir::
        call    __directory_validate
        jr      nc,.valid
        call    __zx_esx_errno
        ld      de,#0
        ret
.valid:
        push    ix
        push    hl
        pop     ix
        ld      de,#2                   ; private native-entry buffer
        add     hl,de
        call    __zx_esx_f_readdir
        jr      c,.native_error
        or      a
        jr      z,.end_with_frame
        push    ix
        pop     hl
        ld      de,#2
        add     hl,de                   ; native record
        push    ix
        pop     de
        ld      bc,#24
        ex      de,hl
        add     hl,bc
        ex      de,hl                   ; public struct dirent
        call    __directory_convert
        pop     ix
        ret
.native_error:
        pop     ix
        call    __zx_esx_error
.end:
        ld      de,#0
        ret
.end_with_frame:
        pop     ix
        jr      .end
