        ; Return POSIX status for an esxDOS file descriptor.
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fstat
        .optsdcc -mz80 sdcccall(1)

        .globl  _fstat
        .globl  _enter_critical_section
        .globl  _leave_critical_section
        .globl  __zx_esx_fd
        .globl  __zx_esx_buffer
        .globl  __zx_esx_errno
        .globl  __zx_esx_error
        .globl  __zx_esx_stat_convert
        .globl  __zx_esx_f_fstat

        .area   _CODE

        ; _fstat
        ; inputs: HL = fd, DE = struct stat pointer (sdcccall(1)).
        ; outputs: DE = 0 or -1 with errno set.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
_fstat::
        call    _enter_critical_section
        push    ix
        ld      ix,#0
        add     ix,sp
        ; IX-2: 14-byte public status pointer
        push    de
        push    hl                      ; IX-4: descriptor
        ex      de,hl
        ld      bc,#14
        call    __zx_esx_buffer
        jp      c,.esx_fstat_errno
        ld      l,-4(ix)
        ld      h,-3(ix)
        call    __zx_esx_fd
        jr      c,.esx_fstat_errno
        push    af                      ; IX-5: native handle
        ld      hl,#-12
        add     hl,sp
        ; IX-18: raw 11-byte status, pad byte
        ld      sp,hl
        ld      a,-5(ix)
        call    __zx_esx_f_fstat
        jr      c,.esx_fstat_native_error
        push    ix
        pop     hl
        ld      bc,#-18
        add     hl,bc
        ld      e,-2(ix)
        ld      d,-1(ix)
        call    __zx_esx_stat_convert
.return:
        ld      sp,ix
        pop     ix
        jp      _leave_critical_section

.esx_fstat_errno:
        call    __zx_esx_errno
        jr      .return
.esx_fstat_native_error:
        call    __zx_esx_error
        jr      .return
