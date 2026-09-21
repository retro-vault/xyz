        ; lseek.s -- checked POSIX seek over esxDOS's unsigned seek API
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module lseek
        .optsdcc -mz80 sdcccall(1)

        .globl  _lseek
        .globl  _enter_critical_section
        .globl  _leave_critical_section
        .globl  __zx_esx_fd
        .globl  __zx_esx_errno
        .globl  __zx_esx_error
        .globl  __zx_esx_f_fstat
        .globl  __zx_esx_f_fgetpos
        .globl  __zx_esx_f_seek

        .area   _CODE

        ; _lseek
        ; inputs: HL = fd (sdcccall(1)); offset at 4(ix)..7(ix),
        ; whence at 8(ix)..9(ix) after PUSH IX.
        ; outputs: HL:DE = new offset or -1; DE is the low word.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
_lseek::
        call    _enter_critical_section
        ; HL=fd; offset and whence are stack arguments. The long return
        ; is
        ; HL:DE, with DE holding the low word, as in sdcccall(1).
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __zx_esx_fd
        jp      c,.esx_seek_errno
        push    af                      ; IX-1: native handle
        ld      hl,#-12
        add     hl,sp
        ; IX-14: raw 11-byte fstat, pad byte
        ld      sp,hl

        ld      a,9(ix)
        or      a
        jp      nz,.esx_seek_invalid
        ld      a,8(ix)
        cp      #3
        jr      nc,.esx_seek_invalid
        or      a
        jr      z,.esx_seek_from_start
        dec     a
        jr      nz,.esx_seek_from_end

        ld      a,-1(ix)
        call    __zx_esx_f_fgetpos
        jr      c,.esx_seek_native_error
        jr      .esx_seek_check_base

.esx_seek_from_end:
        push    ix
        pop     hl
        ld      bc,#-14
        add     hl,bc
        ld      a,-1(ix)
        call    __zx_esx_f_fstat
        jr      c,.esx_seek_native_error
        ; raw status size starts at byte 7
        ld      e,-7(ix)
        ld      d,-6(ix)
        ld      c,-5(ix)
        ld      b,-4(ix)
.esx_seek_check_base:
        bit     7,b
        jr      nz,.esx_seek_overflow
        jr      .esx_seek_add_offset

.esx_seek_from_start:
        ld      bc,#0
        ld      de,#0
.esx_seek_add_offset:
        ld      a,e
        add     a,4(ix)
        ld      e,a
        ld      a,d
        adc     a,5(ix)
        ld      d,a
        ld      a,c
        adc     a,6(ix)
        ld      c,a
        ld      a,b
        adc     a,7(ix)
        ld      b,a
        ; The base is nonnegative. A negative addend must carry out of
        ; the
        ; unsigned representation to produce a nonnegative signed
        ; result.
        bit     7,7(ix)
        jr      z,.esx_seek_positive_offset
        jr      nc,.esx_seek_invalid
        jr      .esx_seek_absolute
.esx_seek_positive_offset:
        bit     7,b
        jr      nz,.esx_seek_overflow
.esx_seek_absolute:
        ; external IXL=absolute seek mode
        ld      hl,#0
        ld      a,-1(ix)
        call    __zx_esx_f_seek
        jr      c,.esx_seek_native_error
        ; Query the actual position. Firmware may clamp at EOF, and
        ; older
        ; esxDOS versions did not document a reliable seek return value.
        ld      a,-1(ix)
        call    __zx_esx_f_fgetpos
        jr      c,.esx_seek_native_error
        bit     7,b
        jr      nz,.esx_seek_overflow
        ld      h,b
        ld      l,c
.return:
        ld      sp,ix
        pop     ix
        jp      _leave_critical_section

.esx_seek_invalid:
        ; EINVAL: bad whence/negative result
        ld      a,#22
        jr      .esx_seek_errno
.esx_seek_overflow:
        ; EOVERFLOW: off_t cannot represent it
        ld      a,#75
.esx_seek_errno:
        call    __zx_esx_errno
        jr      .return
.esx_seek_native_error:
        call    __zx_esx_error
        jr      .return
