        ; POSIX rename over resident esxDOS firmware.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module rename
        .optsdcc -mz80 sdcccall(1)

        .globl  _rename
        .globl  __zx_esx_path
        .globl  __zx_esx_errno
        .globl  __zx_esx_error
        .globl  __zx_esx_f_rename

        .area   _CODE

        ; _rename
        ; inputs: HL = old path, DE = new path (sdcccall(1)).
        ; outputs: DE = 0 or -1 with errno set.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
_rename::
        call    __zx_esx_path
        jp      c,__zx_esx_errno
        ex      de,hl
        call    __zx_esx_path
        ex      de,hl
        jp      c,__zx_esx_errno
        ; Preserve the firmware's rename semantics. In particular, never
        ; delete the destination in advance to emulate POSIX
        ; replacement.
        ld      a,#0x2a
        call    __zx_esx_f_rename
        jp      c,__zx_esx_error
        ld      de,#0
        ret
