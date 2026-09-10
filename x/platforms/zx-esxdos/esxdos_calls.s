        ; esxDOS external RAM ABI: the firmware substitutes IX for HL.
        ; Keep XCC's IX/IY homes intact and translate the result back to
        ; HL.
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module esxdos_calls
        .optsdcc -mz80 sdcccall(1)

        .globl  _zx_esxdos_version
        .globl  __zx_esx_error
        .globl  __zx_esx_m_dosversion
        .globl  __zx_esx_f_open
        .globl  __zx_esx_f_close
        .globl  __zx_esx_f_sync
        .globl  __zx_esx_f_read
        .globl  __zx_esx_f_write
        .globl  __zx_esx_f_seek
        .globl  __zx_esx_f_fgetpos
        .globl  __zx_esx_f_fstat
        .globl  __zx_esx_f_getcwd
        .globl  __zx_esx_f_chdir
        .globl  __zx_esx_f_mkdir
        .globl  __zx_esx_f_rmdir
        .globl  __zx_esx_f_stat
        .globl  __zx_esx_f_unlink
        .globl  __zx_esx_f_rename

        .area   _CODE

        ; _zx_esxdos_version
        ; inputs: initialized esxDOS firmware (sdcccall(1)).
        ; outputs: DE = firmware BCD version or -1 with errno set.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
_zx_esxdos_version::
        call    __zx_esx_m_dosversion
        jp      c,__zx_esx_error
        ex      de,hl
        ret

        ; __zx_esx_m_dosversion
        ; inputs: none.
        ; outputs: HL = BCD version, BC/DE = firmware signature.
        ; CY set and A = native error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_m_dosversion::
        push    ix
        push    iy
        push    hl
        pop     ix
        rst     0x08
        .db     0x88
        push    ix
        pop     hl
        pop     iy
        pop     ix
        ret

        ; __zx_esx_f_open
        ; inputs: A = drive, HL = path, B = native open mode.
        ; outputs: A = native handle on success.
        ; CY set and A = native error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_f_open::
        push    ix
        push    iy
        push    hl
        pop     ix
        rst     0x08
        .db     0x9a
        push    ix
        pop     hl
        pop     iy
        pop     ix
        ret

        ; __zx_esx_f_close
        ; inputs: A = native file handle.
        ; outputs: native results; use CY for success/failure.
        ; CY set and A = native error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_f_close::
        push    ix
        push    iy
        push    hl
        pop     ix
        rst     0x08
        .db     0x9b
        push    ix
        pop     hl
        pop     iy
        pop     ix
        ret

        ; __zx_esx_f_sync
        ; inputs: A = native file handle.
        ; outputs: native results; use CY for success/failure.
        ; CY set and A = native error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_f_sync::
        push    ix
        push    iy
        push    hl
        pop     ix
        rst     0x08
        .db     0x9c
        push    ix
        pop     hl
        pop     iy
        pop     ix
        ret

        ; __zx_esx_f_read
        ; inputs: A = handle, HL = buffer, BC = byte count.
        ; outputs: BC = actual bytes read on success.
        ; CY set and A = native error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_f_read::
        push    ix
        push    iy
        push    hl
        pop     ix
        rst     0x08
        .db     0x9d
        push    ix
        pop     hl
        pop     iy
        pop     ix
        ret

        ; __zx_esx_f_write
        ; inputs: A = handle, HL = buffer, BC = byte count.
        ; outputs: BC = actual bytes written on success.
        ; CY set and A = native error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_f_write::
        push    ix
        push    iy
        push    hl
        pop     ix
        rst     0x08
        .db     0x9e
        push    ix
        pop     hl
        pop     iy
        pop     ix
        ret

        ; __zx_esx_f_seek
        ; inputs: A = handle, BC:DE = offset, L = native seek mode.
        ; outputs: native results; use CY for success/failure.
        ; CY set and A = native error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_f_seek::
        push    ix
        push    iy
        push    hl
        pop     ix
        rst     0x08
        .db     0x9f
        push    ix
        pop     hl
        pop     iy
        pop     ix
        ret

        ; __zx_esx_f_fgetpos
        ; inputs: A = native file handle.
        ; outputs: BC:DE = current position on success.
        ; CY set and A = native error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_f_fgetpos::
        push    ix
        push    iy
        push    hl
        pop     ix
        rst     0x08
        .db     0xa0
        push    ix
        pop     hl
        pop     iy
        pop     ix
        ret

        ; __zx_esx_f_fstat
        ; inputs: A = handle, HL = native status buffer.
        ; outputs: native results; use CY for success/failure.
        ; CY set and A = native error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_f_fstat::
        push    ix
        push    iy
        push    hl
        pop     ix
        rst     0x08
        .db     0xa1
        push    ix
        pop     hl
        pop     iy
        pop     ix
        ret

        ; __zx_esx_f_getcwd
        ; inputs: A = drive, HL = pathname output buffer.
        ; outputs: native results; use CY for success/failure.
        ; CY set and A = native error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_f_getcwd::
        push    ix
        push    iy
        push    hl
        pop     ix
        rst     0x08
        .db     0xa8
        push    ix
        pop     hl
        pop     iy
        pop     ix
        ret

        ; __zx_esx_f_chdir
        ; inputs: A = drive, HL = path.
        ; outputs: native results; use CY for success/failure.
        ; CY set and A = native error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_f_chdir::
        push    ix
        push    iy
        push    hl
        pop     ix
        rst     0x08
        .db     0xa9
        push    ix
        pop     hl
        pop     iy
        pop     ix
        ret

        ; __zx_esx_f_mkdir
        ; inputs: A = drive, HL = path.
        ; outputs: native results; use CY for success/failure.
        ; CY set and A = native error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_f_mkdir::
        push    ix
        push    iy
        push    hl
        pop     ix
        rst     0x08
        .db     0xaa
        push    ix
        pop     hl
        pop     iy
        pop     ix
        ret

        ; __zx_esx_f_rmdir
        ; inputs: A = drive, HL = path.
        ; outputs: native results; use CY for success/failure.
        ; CY set and A = native error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_f_rmdir::
        push    ix
        push    iy
        push    hl
        pop     ix
        rst     0x08
        .db     0xab
        push    ix
        pop     hl
        pop     iy
        pop     ix
        ret

        ; __zx_esx_f_stat
        ; inputs: A = drive, HL = path, DE = native status buffer.
        ; outputs: native results; use CY for success/failure.
        ; CY set and A = native error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_f_stat::
        push    ix
        push    iy
        push    hl
        pop     ix
        rst     0x08
        .db     0xac
        push    ix
        pop     hl
        pop     iy
        pop     ix
        ret

        ; __zx_esx_f_unlink
        ; inputs: A = drive, HL = path.
        ; outputs: native results; use CY for success/failure.
        ; CY set and A = native error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_f_unlink::
        push    ix
        push    iy
        push    hl
        pop     ix
        rst     0x08
        .db     0xad
        push    ix
        pop     hl
        pop     iy
        pop     ix
        ret

        ; __zx_esx_f_rename
        ; inputs: A = drive, HL = old path, DE = new path.
        ; outputs: native results; use CY for success/failure.
        ; CY set and A = native error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_f_rename::
        push    ix
        push    iy
        push    hl
        pop     ix
        rst     0x08
        .db     0xb0
        push    ix
        pop     hl
        pop     iy
        pop     ix
        ret
