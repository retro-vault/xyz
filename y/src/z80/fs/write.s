        ; Write an esxDOS file without importing terminal support.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module write
        .optsdcc -mz80 sdcccall(1)

        .globl  _write
        .globl  _enter_critical_section
        .globl  _leave_critical_section
        .globl  _lseek
        .globl  __zx_esx_fd
        .globl  __zx_esx_source
        .globl  __zx_esx_errno
        .globl  __zx_esx_error
        .globl  __zx_esx_f_write

        .area   _CODE

        ; inputs: HL = fd, DE = buffer, count at 4(ix) after PUSH IX.
        ; output: DE = count/-1; clobbers: af, bc, de, hl.
        ; IX/IY preserved. Append seeks to the current end on each write.
_write::
        call    _enter_critical_section
        push    ix
        ld      ix,#0
        add     ix,sp
        push    de                      ; -2: buffer
        push    hl                      ; -4: fd
        call    __zx_esx_fd
        jr      c,.write_errno
        ld      a,c
        and     #3
        jr      z,.write_bad_fd
        push    hl                      ; -6: descriptor entry
        ld      c,4(ix)
        ld      b,5(ix)
        bit     7,b
        jr      z,.write_count
        ld      bc,#0x7fff
.write_count:
        ld      l,-2(ix)
        ld      h,-1(ix)
        call    __zx_esx_source
        jr      c,.write_errno
        ld      a,b
        or      c
        jr      z,.write_zero
        push    bc                      ; -8: requested/clamped count
        ld      l,-6(ix)
        ld      h,-5(ix)
        inc     hl
        bit     2,(hl)
        jr      z,.write_transfer
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
        jr      nz,.write_return
.write_transfer:
        ld      c,-8(ix)
        ld      b,-7(ix)
        ld      e,-6(ix)
        ld      d,-5(ix)
        ld      a,(de)
        ld      l,-2(ix)
        ld      h,-1(ix)
        call    .write_source
        jr      c,.write_native_error
        ld      e,c
        ld      d,b
        jr      .write_return
.write_zero:
        ld      de,#0
        jr      .write_return
.write_bad_fd:
        ld      a,#9
.write_errno:
        call    __zx_esx_errno
        jr      .write_return
.write_native_error:
        call    __zx_esx_error
.write_return:
        ld      sp,ix
        pop     ix
        jp      _leave_critical_section

        ; inputs: A = handle, HL = readable source, BC = nonzero count.
        ; outputs: BC = actual count, CY clear; or native A/CY error.
        ; RAM sources use one native call. ROM sources use 128 bytes of
        ; private stack while the firmware hides the application ROM.
.write_source:
        push    af
        ld      a,h
        cp      #0x40
        jr      c,.write_rom
        pop     af
        jp      __zx_esx_f_write
.write_rom:
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
.write_chunk:
        ld      c,-6(ix)
        ld      b,-5(ix)
        ld      a,b
        or      a
        jr      nz,.write_chunk_full
        ld      a,c
        cp      #128
        jr      c,.write_chunk_ready
.write_chunk_full:
        ld      bc,#128
.write_chunk_ready:
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
        jr      c,.write_chunk_return
        ld      l,-10(ix)
        ld      h,-9(ix)
        or      a
        sbc     hl,bc
        jr      c,.write_chunk_invalid
        push    af                      ; short-write comparison
        ld      l,-8(ix)
        ld      h,-7(ix)
        add     hl,bc
        ld      -8(ix),l
        ld      -7(ix),h
        pop     af
        jr      nz,.write_chunk_done
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
        jr      nz,.write_chunk
.write_chunk_done:
        ld      c,-8(ix)
        ld      b,-7(ix)
        or      a
        jr      .write_chunk_return
.write_chunk_invalid:
        ld      a,#6                    ; native EIO
        scf
.write_chunk_return:
        ; Firmware BC is unreliable on errors, including after earlier
        ; chunks succeeded. Preserve the backend's -1/error contract.
        ld      sp,ix
        pop     ix
        ret
