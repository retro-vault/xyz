        ; Flush an open esxDOS file to its device.
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fsync
        .optsdcc -mz80 sdcccall(1)

        .globl  _fsync
        .globl  __zx_esx_fd
        .globl  __zx_esx_errno
        .globl  __zx_esx_error
        .globl  __zx_esx_f_sync

        .area   _CODE

        ; inputs: HL = fd; output: DE = 0 or -1.
        ; clobbers: af, bc, de, hl. IX/IY preserved.
_fsync::
        ld      a,h
        or      a
        jr      nz,.esx_sync_file
        ld      a,l
        cp      #3
        jr      nc,.esx_sync_file
        ld      a,#22
        jp      __zx_esx_errno
.esx_sync_file:
        call    __zx_esx_fd
        jp      c,__zx_esx_errno
        call    __zx_esx_f_sync
        jp      c,__zx_esx_error
        ld      de,#0
        ret
