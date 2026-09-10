        ; Write the bitmap console or an esxDOS file.
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module write
        .optsdcc -mz80 sdcccall(1)

        .globl  _write
        .globl  _lseek
        .globl  _zx_console_putc_a
        .globl  __zx_esx_fd
        .globl  __zx_esx_source
        .globl  __zx_esx_errno
        .globl  __zx_esx_error
        .globl  __zx_esx_f_write

        .area   _CODE

        ; inputs: HL = fd, DE = buffer, count at 4(ix) after PUSH IX.
        ; output: DE = count/-1; clobbers: af, bc, de, hl.
        ; IX/IY preserved. Append seeks to the current end on every
        ; write.
_write::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    de                      ; -2: buffer
        push    hl                      ; -4: fd
        ld      a,h
        or      a
        jr      nz,.esx_write_file
        ld      a,l
        cp      #1
        jr      z,.esx_write_console
        cp      #2
        jr      z,.esx_write_console
.esx_write_file:
        call    __zx_esx_fd
        jp      c,.esx_write_errno
        ld      a,c
        and     #3
        jp      z,.esx_write_bad_fd
        push    hl                      ; -6: descriptor entry
        call    .esx_write_buffer
        jp      c,.esx_write_errno
        ld      a,b
        or      c
        jp      z,.esx_write_zero
        push    bc                      ; -8: requested/clamped count
        ld      l,-6(ix)
        ld      h,-5(ix)
        inc     hl
        bit     2,(hl)
        jr      z,.esx_write_transfer
        ld      hl,#2                   ; SEEK_END
        push    hl
        ld      hl,#0
        push    hl                      ; offset high
        push    hl                      ; offset low
        ld      l,-4(ix)
        ld      h,-3(ix)
        call    _lseek
        pop     bc
        pop     bc
        pop     bc
        bit     7,h
        jr      nz,.esx_write_return
.esx_write_transfer:
        ld      c,-8(ix)
        ld      b,-7(ix)
        ld      e,-6(ix)
        ld      d,-5(ix)
        ld      a,(de)
        ld      l,-2(ix)
        ld      h,-1(ix)
        call    .esx_write_source
        jr      c,.esx_write_native_error
        ld      e,c
        ld      d,b
        jr      .esx_write_return
.esx_write_console:
        call    .esx_write_buffer
        jr      c,.esx_write_errno
        push    bc
.esx_write_loop:
        ld      a,b
        or      c
        jr      z,.esx_write_console_done
        push    bc
        push    hl
        ld      a,(hl)
        call    _zx_console_putc_a
        pop     hl
        pop     bc
        inc     hl
        dec     bc
        jr      .esx_write_loop
.esx_write_console_done:
        pop     de
        jr      .esx_write_return
.esx_write_zero:
        ld      de,#0
        jr      .esx_write_return
.esx_write_bad_fd:
        ld      a,#9
.esx_write_errno:
        call    __zx_esx_errno
        jr      .esx_write_return
.esx_write_native_error:
        call    __zx_esx_error
.esx_write_return:
        ld      sp,ix
        pop     ix
        ret

.esx_write_buffer:
        ld      c,4(ix)
        ld      b,5(ix)
        bit     7,b
        jr      z,.esx_write_count
        ld      bc,#0x7fff
.esx_write_count:
        ld      l,-2(ix)
        ld      h,-1(ix)
        jp      __zx_esx_source

        ; .esx_write_source
        ; inputs: A = handle, HL = readable source, BC = nonzero count.
        ; outputs: BC = actual count, CY clear; or native A/CY error.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
        ; RAM sources use one native call. ROM sources use 128 bytes of
        ; private stack storage while the firmware hides the base ROM.
.esx_write_source:
        push    af
        ld      a,h
        cp      #0x40
        jr      c,.esx_write_rom
        pop     af
        jp      __zx_esx_f_write
.esx_write_rom:
        pop     af
        push    ix
        ld      ix,#0
        add     ix,sp
        push    af                      ; IX-2: native handle
        push    hl                      ; IX-4: next source byte
        push    bc                      ; IX-6: remaining count
        ld      hl,#0
        push    hl                      ; IX-8: completed count
        push    hl                      ; IX-10: current chunk size
        ld      hl,#-128
        add     hl,sp
        ld      sp,hl                   ; IX-138: private RAM buffer
.esx_write_chunk:
        ld      c,-6(ix)
        ld      b,-5(ix)
        ld      a,b
        or      a
        jr      nz,.esx_write_chunk_full
        ld      a,c
        cp      #128
        jr      c,.esx_write_chunk_ready
.esx_write_chunk_full:
        ld      bc,#128
.esx_write_chunk_ready:
        ld      -10(ix),c
        ld      -9(ix),b
        push    bc
        push    ix
        pop     hl
        ld      de,#-138
        add     hl,de
        ex      de,hl
        ld      l,-4(ix)
        ld      h,-3(ix)
        ldir
        pop     bc
        push    ix
        pop     hl
        ld      de,#-138
        add     hl,de
        ld      a,-1(ix)
        call    __zx_esx_f_write
        jr      c,.esx_write_chunk_return
        ld      l,-10(ix)
        ld      h,-9(ix)
        or      a
        sbc     hl,bc
        jr      c,.esx_write_chunk_invalid
        push    af                      ; short-write comparison
        ld      l,-8(ix)
        ld      h,-7(ix)
        add     hl,bc
        ld      -8(ix),l
        ld      -7(ix),h
        pop     af
        jr      nz,.esx_write_chunk_done
        ld      l,-4(ix)
        ld      h,-3(ix)
        add     hl,bc
        ld      -4(ix),l
        ld      -3(ix),h
        ld      l,-6(ix)
        ld      h,-5(ix)
        or      a
        sbc     hl,bc
        ld      -6(ix),l
        ld      -5(ix),h
        ld      a,h
        or      l
        jr      nz,.esx_write_chunk
.esx_write_chunk_done:
        ld      c,-8(ix)
        ld      b,-7(ix)
        or      a
        jr      .esx_write_chunk_return
.esx_write_chunk_invalid:
        ld      a,#6                    ; native EIO
        scf
.esx_write_chunk_return:
        ; Firmware BC is unreliable on errors, including after earlier
        ; chunks succeeded. Preserve the backend's -1/error contract.
        ld      sp,ix
        pop     ix
        ret
