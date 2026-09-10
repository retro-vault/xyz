        ; Read console input or an esxDOS file, preserving XCC homes.
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module read
        .optsdcc -mz80 sdcccall(1)

        .globl  _read
        .globl  _getchar
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
        push    ix
        ld      ix,#0
        add     ix,sp
        push    de                      ; -2: buffer
        push    hl                      ; -4: fd
        ld      a,h
        or      l
        jr      z,.esx_read_console
        call    __zx_esx_fd
        jp      c,.esx_read_errno
        ld      a,c
        and     #3
        cp      #1
        jr      z,.esx_read_bad_fd
        push    hl                      ; -6: descriptor entry
        call    .esx_read_buffer
        jp      c,.esx_read_errno
        ld      a,b
        or      c
        jr      z,.esx_read_zero
        ld      e,-6(ix)
        ld      d,-5(ix)
        ld      a,(de)
        call    __zx_esx_f_read
        jr      c,.esx_read_native_error
        ld      e,c
        ld      d,b
        jr      .esx_read_return
.esx_read_console:
        call    .esx_read_buffer
        jr      c,.esx_read_errno
        push    bc                      ; -6: requested/clamped count
.esx_read_loop:
        ld      a,b
        or      c
        jr      z,.esx_read_console_done
        push    bc
        push    hl
        call    _getchar
        ld      a,e
        pop     hl
        pop     bc
        ld      (hl),a
        inc     hl
        dec     bc
        jr      .esx_read_loop
.esx_read_console_done:
        pop     de
        jr      .esx_read_return
.esx_read_zero:
        ld      de,#0
        jr      .esx_read_return
.esx_read_bad_fd:
        ld      a,#9
.esx_read_errno:
        call    __zx_esx_errno
        jr      .esx_read_return
.esx_read_native_error:
        call    __zx_esx_error
.esx_read_return:
        ld      sp,ix
        pop     ix
        ret

        ; Return HL buffer, BC clamped count, carry/A validation result.
.esx_read_buffer:
        ld      c,4(ix)
        ld      b,5(ix)
        bit     7,b
        jr      z,.esx_read_count
        ld      bc,#0x7fff
.esx_read_count:
        ld      l,-2(ix)
        ld      h,-1(ix)
        jp      __zx_esx_buffer
