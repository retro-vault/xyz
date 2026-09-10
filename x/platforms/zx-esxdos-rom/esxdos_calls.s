        ; ROM callers use RAM gates for the external esxDOS ABI.
        ; The firmware substitutes IX for HL; ROM paths are copied.
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

        .globl  __zx_esx_gate_88
        .globl  __zx_esx_gate_9a
        .globl  __zx_esx_gate_9b
        .globl  __zx_esx_gate_9c
        .globl  __zx_esx_gate_9d
        .globl  __zx_esx_gate_9e
        .globl  __zx_esx_gate_9f
        .globl  __zx_esx_gate_a0
        .globl  __zx_esx_gate_a1
        .globl  __zx_esx_gate_a8
        .globl  __zx_esx_gate_a9
        .globl  __zx_esx_gate_aa
        .globl  __zx_esx_gate_ab
        .globl  __zx_esx_gate_ac
        .globl  __zx_esx_gate_ad
        .globl  __zx_esx_gate_b0

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
        call    __zx_esx_gate_88
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
        push    iy
        ld      iy,#__zx_esx_gate_9a
        jp      .esx_path_call

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
        call    __zx_esx_gate_9b
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
        call    __zx_esx_gate_9c
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
        call    __zx_esx_gate_9d
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
        call    __zx_esx_gate_9e
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
        call    __zx_esx_gate_9f
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
        call    __zx_esx_gate_a0
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
        call    __zx_esx_gate_a1
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
        call    __zx_esx_gate_a8
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
        push    iy
        ld      iy,#__zx_esx_gate_a9
        jp      .esx_path_call

        ; __zx_esx_f_mkdir
        ; inputs: A = drive, HL = path.
        ; outputs: native results; use CY for success/failure.
        ; CY set and A = native error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_f_mkdir::
        push    iy
        ld      iy,#__zx_esx_gate_aa
        jp      .esx_path_call

        ; __zx_esx_f_rmdir
        ; inputs: A = drive, HL = path.
        ; outputs: native results; use CY for success/failure.
        ; CY set and A = native error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_f_rmdir::
        push    iy
        ld      iy,#__zx_esx_gate_ab
        jp      .esx_path_call

        ; __zx_esx_f_stat
        ; inputs: A = drive, HL = path, DE = native status buffer.
        ; outputs: native results; use CY for success/failure.
        ; CY set and A = native error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_f_stat::
        push    iy
        ld      iy,#__zx_esx_gate_ac
        jp      .esx_path_call

        ; __zx_esx_f_unlink
        ; inputs: A = drive, HL = path.
        ; outputs: native results; use CY for success/failure.
        ; CY set and A = native error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_f_unlink::
        push    iy
        ld      iy,#__zx_esx_gate_ad
        jp      .esx_path_call

        ; __zx_esx_f_rename
        ; inputs: A = drive, HL = old path, DE = new path.
        ; outputs: native results; use CY for success/failure.
        ; CY set and A = native error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_f_rename::
        push    iy
        ld      iy,#__zx_esx_gate_b0
        jp      .esx_path_call

        ; .esx_path_call
        ; inputs: A = drive, HL = checked path, DE = second pointer,
        ; B = open mode, IY = RAM gate; caller IY is already stacked.
        ; outputs: native AF/BC/DE/HL results; preserves caller IX/IY.
        ; ROM paths use their length plus NUL in private stack storage,
        ; at most 256 bytes each. RAM paths pass directly to firmware.
.esx_path_call:
        push    ix
        ld      ix,#0
        add     ix,sp
        push    af                      ; IX-2: drive and flags
        push    bc                      ; IX-4: native mode
        push    de                      ; IX-6: second pointer
        push    hl                      ; IX-8: first path
        push    iy                      ; IX-10: selected RAM gate
        ld      a,h
        cp      #0x40
        jr      nc,.esx_path_second
        call    .esx_path_size
        ld      hl,#0
        or      a
        sbc     hl,bc
        add     hl,sp
        ld      sp,hl
        ex      de,hl
        ld      l,-8(ix)
        ld      h,-7(ix)
        call    .esx_path_copy
        ld      -8(ix),l
        ld      -7(ix),h
.esx_path_second:
        ld      l,-10(ix)
        ld      h,-9(ix)
        ld      de,#__zx_esx_gate_b0
        or      a
        sbc     hl,de
        jr      nz,.esx_path_ready
        ld      a,-5(ix)
        cp      #0x40
        jr      nc,.esx_path_ready
        ld      l,-6(ix)
        ld      h,-5(ix)
        call    .esx_path_size
        ld      hl,#0
        or      a
        sbc     hl,bc
        add     hl,sp
        ld      sp,hl
        ex      de,hl
        ld      l,-6(ix)
        ld      h,-5(ix)
        call    .esx_path_copy
        ld      -6(ix),l
        ld      -5(ix),h
.esx_path_ready:
        ld      l,-10(ix)
        ld      h,-9(ix)
        push    hl
        pop     iy
        ld      c,-4(ix)
        ld      b,-3(ix)
        ld      e,-6(ix)
        ld      d,-5(ix)
        ld      l,-2(ix)
        ld      h,-1(ix)
        push    hl
        pop     af
        ld      l,-8(ix)
        ld      h,-7(ix)
        push    ix                      ; retain frame across firmware
        push    hl
        pop     ix
        call    .esx_path_gate
        push    ix
        pop     hl
        pop     ix
        ld      sp,ix
        pop     ix
        pop     iy
        ret
.esx_path_gate:
        jp      (iy)

        ; HL = validated source. Return BC = length including NUL.
        ; Clobbers AF/HL; the caller reloads its saved source pointer.
.esx_path_size:
        ld      bc,#256
        xor     a
        cpir
        ld      hl,#256
        or      a
        sbc     hl,bc
        ld      b,h
        ld      c,l
        ret

        ; HL = validated source, DE = private RAM buffer, BC = size.
        ; Return HL = buffer; copy the complete path including NUL.
.esx_path_copy:
        push    de
        ldir
        pop     hl
        ret
