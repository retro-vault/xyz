        ; Read and convert the next entry from a YOS directory.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module readdir
        .optsdcc -mz80 sdcccall(1)

        .globl  _readdir
        .globl  __critical_call
        .globl  __directory_validate
        .globl  __directory_convert
        .globl  __next_fs_errno
        .globl  __next_fs_error
        .globl  __next_fs_f_readdir

        .globl __p3_directory_name

        .area   _CODE

        ; input: HL = DIR pointer; output: DE = dirent pointer or NULL.
_readdir::
        call    __critical_call
        call    __directory_validate
        jr      nc,.valid
        call    __next_fs_errno
        ld      de,#0
        ret
.valid:
        push    ix
        push    hl
        pop     ix
        ld      de,#2                   ; private native-entry buffer
        add     hl,de
        call    __next_fs_f_readdir
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
        ex      de,hl                   ; public yos_directory_entry_t
        call    __directory_convert
        push    de                      ; preserve public entry pointer
        ld      hl,#23
        add     hl,de                   ; appended display name
        ex      de,hl
        ld      bc,#10
        add     hl,bc                   ; DOS alias
.name_copy:
        ld      a,(hl)
        ld      (de),a
        inc     hl
        inc     de
        or      a
        jr      nz,.name_copy
        pop     hl
        push    hl
        ld      de,#23
        add     hl,de
        ld      a,(ix+0)
        call    __p3_directory_name
        pop     de
        pop     ix
        ret
.native_error:
        pop     ix
        call    __next_fs_error
.end:
        ld      de,#0
        ret
.end_with_frame:
        pop     ix
        jr      .end
