        ; Read an esxDOS file without importing terminal support.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module read
        .optsdcc -mz80 sdcccall(1)

        .globl  _read
        .globl  _enter_critical_section
        .globl  _leave_critical_section
        .globl  __zx_esx_fd
        .globl  __zx_esx_buffer
        .globl  __zx_esx_errno
        .globl  __zx_esx_error
        .globl  __zx_esx_f_read

        .area   _CODE

        ; inputs: HL = fd, DE = buffer, count at 4(ix) after PUSH IX.
        ; output: DE = count/EOF/-1; clobbers: af, bc, de, hl.
        ; IX/IY preserved. Limit transfers to signed ssize_t's maximum.
_read::
        call    _enter_critical_section
        push    ix
        ld      ix,#0
        add     ix,sp
        push    de                      ; -2: buffer
        push    hl                      ; -4: fd
        call    __zx_esx_fd
        jr      c,.read_errno
        ld      a,c
        and     #3
        cp      #1
        jr      z,.read_bad_fd
        push    hl                      ; -6: descriptor entry
        ld      c,4(ix)
        ld      b,5(ix)
        bit     7,b
        jr      z,.read_count
        ld      bc,#0x7fff
.read_count:
        ld      l,-2(ix)
        ld      h,-1(ix)
        call    __zx_esx_buffer
        jr      c,.read_errno
        ld      a,b
        or      c
        jr      z,.read_zero
        ld      e,-6(ix)
        ld      d,-5(ix)
        ld      a,(de)
        call    __zx_esx_f_read
        jr      c,.read_native_error
        ld      e,c
        ld      d,b
        jr      .read_return
.read_zero:
        ld      de,#0
        jr      .read_return
.read_bad_fd:
        ld      a,#9
.read_errno:
        call    __zx_esx_errno
        jr      .read_return
.read_native_error:
        call    __zx_esx_error
.read_return:
        ld      sp,ix
        pop     ix
        jp      _leave_critical_section
