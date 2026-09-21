        ; POSIX getcwd over resident esxDOS firmware.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module getcwd
        .optsdcc -mz80 sdcccall(1)

        .globl  _getcwd
        .globl  __zx_esx_errno
        .globl  __zx_esx_error
        .globl  __zx_esx_f_getcwd

        .area   _CODE

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
        jr      z,.esx_getcwd_invalid
        ld      a,d
        or      e
        jr      z,.esx_getcwd_invalid
        ld      a,h
        cp      #0x40
        jr      c,.esx_getcwd_fault
        dec     de
        add     hl,de
        jr      c,.esx_getcwd_fault

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
