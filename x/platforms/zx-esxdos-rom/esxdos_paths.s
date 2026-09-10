        ; esxdos_paths.s -- pathname services for resident esxDOS 0.8.9
        ;
        ; Firmware thunks translate the external IX pointer convention
        ; to
        ; HL and preserve the C caller's IX/IY. All temporary storage is
        ; on
        ; the stack. Firmware errors are translated by the shared
        ; backend.

        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module esxdos_paths
        .optsdcc -mz80 sdcccall(1)

        .globl  _unlink
        .globl  _rename
        .globl  _chdir
        .globl  _getcwd
        .globl  _mkdir
        .globl  _rmdir
        .globl  _stat
        .globl  __zx_esx_path
        .globl  __zx_esx_errno
        .globl  __zx_esx_error
        .globl  __zx_esx_stat_convert
        .globl  __zx_esx_f_unlink
        .globl  __zx_esx_f_rename
        .globl  __zx_esx_f_chdir
        .globl  __zx_esx_f_getcwd
        .globl  __zx_esx_f_mkdir
        .globl  __zx_esx_f_rmdir
        .globl  __zx_esx_f_stat

        .area   _CODE

        ; _unlink
        ; inputs: HL = NUL-terminated path (sdcccall(1)).
        ; outputs: DE = 0 or -1 with errno set.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
_unlink::
        call    __zx_esx_path
        jp      c,__zx_esx_errno
        ld      a,#0x2a
        call    __zx_esx_f_unlink
        jp      c,__zx_esx_error
        ld      de,#0
        ret

        ; _chdir
        ; inputs: HL = NUL-terminated path (sdcccall(1)).
        ; outputs: DE = 0 or -1 with errno set.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
_chdir::
        call    __zx_esx_path
        jp      c,__zx_esx_errno
        ld      a,#0x2a
        call    __zx_esx_f_chdir
        jp      c,__zx_esx_error
        ld      de,#0
        ret

        ; _mkdir
        ; inputs: HL = path, DE = ignored mode (sdcccall(1)).
        ; outputs: DE = 0 or -1 with errno set.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
_mkdir::
        ; FAT has no POSIX permission bits; the mode argument is
        ; ignored.
        call    __zx_esx_path
        jp      c,__zx_esx_errno
        ld      a,#0x2a
        call    __zx_esx_f_mkdir
        jp      c,__zx_esx_error
        ld      de,#0
        ret

        ; _rmdir
        ; inputs: HL = NUL-terminated path (sdcccall(1)).
        ; outputs: DE = 0 or -1 with errno set.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
_rmdir::
        call    __zx_esx_path
        jp      c,__zx_esx_errno
        ld      a,#0x2a
        call    __zx_esx_f_rmdir
        jp      c,__zx_esx_error
        ld      de,#0
        ret

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

        ; _stat
        ; inputs: HL = path, DE = struct stat pointer (sdcccall(1)).
        ; outputs: DE = 0 or -1 with errno set.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
_stat::
        call    __zx_esx_path
        jp      c,__zx_esx_errno
        ; The complete 14-byte public object must lie in writable RAM.
        ld      a,d
        cp      #0x40
        jr      c,.esx_stat_fault
        push    hl
        push    de
        ex      de,hl
        ld      bc,#13
        add     hl,bc
        pop     de
        pop     hl
        jr      c,.esx_stat_fault

        push    ix
        ld      ix,#0
        add     ix,sp
        push    de                      ; IX-2: public status pointer
        push    hl                      ; IX-4: path
        ld      hl,#-12
        add     hl,sp
        ; IX-16: raw 11-byte status, pad byte
        ld      sp,hl
        ex      de,hl
        ld      l,-4(ix)
        ld      h,-3(ix)
        ld      a,#0x2a
        call    __zx_esx_f_stat
        jr      c,.esx_stat_native_error
        push    ix
        pop     hl
        ld      bc,#-16
        add     hl,bc
        ld      e,-2(ix)
        ld      d,-1(ix)
        call    __zx_esx_stat_convert
        ld      sp,ix
        pop     ix
        ret
.esx_stat_native_error:
        call    __zx_esx_error
        ld      sp,ix
        pop     ix
        ret
.esx_stat_fault:
        ld      a,#14                   ; EFAULT
        jp      __zx_esx_errno

        ; _getcwd
        ; inputs: HL = buffer, DE = buffer size (sdcccall(1)).
        ; outputs: DE = buffer or NULL with errno set.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
_getcwd::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    hl                      ; IX-2: caller's output buffer
        push    de                      ; IX-4: caller's buffer size
        ld      a,h
        or      l
        jp      z,.esx_getcwd_invalid
        ld      a,d
        or      e
        jp      z,.esx_getcwd_invalid
        ld      a,h
        cp      #0x40
        jp      c,.esx_getcwd_fault
        dec     de
        add     hl,de
        jp      c,.esx_getcwd_fault

        ; The 0.8.x ABI has no destination-length parameter. Reserve 256
        ; bytes (also covering z88dk's 128-byte 0.8.5 pathname limit),
        ; then
        ; check the returned string before touching the caller's buffer.
        ld      hl,#-256
        add     hl,sp
        ; IX-260: private pathname buffer
        ld      sp,hl
        ld      d,h
        ld      e,l
        inc     de
        ld      (hl),#0xff
        ld      bc,#255
        ldir
        push    ix
        pop     hl
        ld      bc,#-260
        add     hl,bc
        ld      a,#0x2a
        call    __zx_esx_f_getcwd
        jr      c,.esx_getcwd_native_error

        push    ix
        pop     hl
        ld      bc,#-260
        add     hl,bc
        ld      bc,#256
        xor     a
        cpir
        jr      nz,.esx_getcwd_range
        ld      hl,#256
        or      a
        ; count includes the terminating NUL
        sbc     hl,bc
        push    hl
        ld      e,-4(ix)
        ld      d,-3(ix)
        or      a
        sbc     hl,de
        pop     bc                      ; complete copy length
        jr      c,.esx_getcwd_copy
        jr      nz,.esx_getcwd_range
.esx_getcwd_copy:
        push    bc
        push    ix
        pop     hl
        ld      bc,#-260
        add     hl,bc
        pop     bc
        ld      e,-2(ix)
        ld      d,-1(ix)
        ldir
        ld      e,-2(ix)
        ld      d,-1(ix)
        ld      sp,ix
        pop     ix
        ret

.esx_getcwd_invalid:
        ld      a,#22                   ; EINVAL
        jr      .esx_getcwd_errno
.esx_getcwd_fault:
        ld      a,#14                   ; EFAULT
        jr      .esx_getcwd_errno
.esx_getcwd_range:
        ; ERANGE: caller buffer too small
        ld      a,#34
.esx_getcwd_errno:
        call    __zx_esx_errno
        jr      .esx_getcwd_null
.esx_getcwd_native_error:
        call    __zx_esx_error
.esx_getcwd_null:
        ld      de,#0
        ld      hl,#0
        ld      sp,ix
        pop     ix
        ret
