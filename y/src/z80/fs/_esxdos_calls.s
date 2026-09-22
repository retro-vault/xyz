        ; ROM callers use RAM gates for the external esxDOS ABI.
        ; The firmware substitutes IX for HL; ROM paths are copied.
        ; Keep XCC's IX/IY homes intact and translate the result back to
        ; HL.
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module esxdos_calls
        .optsdcc -mz80 sdcccall(1)

        .globl  __zx_esx_path_drive
        .globl  __zx_esx_error
        .globl  _enter_critical_section
        .globl  _leave_critical_section
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
        .globl  __zx_esx_f_opendir
        .globl  __zx_esx_f_readdir
        .globl  __zx_esx_f_rewinddir
        .globl  __zx_esx_disk_info
        .globl  __zx_esx_m_execcmd

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
        .globl  __zx_esx_gate_a3
        .globl  __zx_esx_gate_a4
        .globl  __zx_esx_gate_a7
        .globl  __zx_esx_gate_84
        .globl  __zx_esx_gate_8f

        .globl  __frame_ix

        .area   _CODE

        ; __zx_esx_f_open
        ; inputs: A = drive, HL = path, B = native open mode.
        ; outputs: A = native handle on success.
        ; CY set and A = native error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_f_open::
        call    .select_path
        .dw     __zx_esx_gate_9a

        ; __zx_esx_f_close
        ; inputs: A = native file handle.
        ; outputs: native results; use CY for success/failure.
        ; CY set and A = native error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_f_close::
        call    .select_direct
        .dw     __zx_esx_gate_9b

        ; __zx_esx_f_sync
        ; inputs: A = native file handle.
        ; outputs: native results; use CY for success/failure.
        ; CY set and A = native error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_f_sync::
        call    .select_direct
        .dw     __zx_esx_gate_9c

        ; __zx_esx_f_read
        ; inputs: A = handle, HL = buffer, BC = byte count.
        ; outputs: BC = actual bytes read on success.
        ; CY set and A = native error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_f_read::
        call    .select_direct
        .dw     __zx_esx_gate_9d

        ; __zx_esx_f_write
        ; inputs: A = handle, HL = buffer, BC = byte count.
        ; outputs: BC = actual bytes written on success.
        ; CY set and A = native error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_f_write::
        call    .select_direct
        .dw     __zx_esx_gate_9e

        ; __zx_esx_f_seek
        ; inputs: A = handle, BC:DE = offset, L = native seek mode.
        ; outputs: native results; use CY for success/failure.
        ; CY set and A = native error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_f_seek::
        call    .select_direct
        .dw     __zx_esx_gate_9f

        ; __zx_esx_f_fgetpos
        ; inputs: A = native file handle.
        ; outputs: BC:DE = current position on success.
        ; CY set and A = native error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_f_fgetpos::
        call    .select_direct
        .dw     __zx_esx_gate_a0

        ; __zx_esx_f_fstat
        ; inputs: A = handle, HL = native status buffer.
        ; outputs: native results; use CY for success/failure.
        ; CY set and A = native error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_f_fstat::
        call    .select_direct
        .dw     __zx_esx_gate_a1

        ; __zx_esx_f_getcwd
        ; inputs: A = drive, HL = pathname output buffer.
        ; outputs: native results; use CY for success/failure.
        ; CY set and A = native error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_f_getcwd::
        call    .select_direct
        .dw     __zx_esx_gate_a8

        ; __zx_esx_f_chdir
        ; inputs: A = drive, HL = path.
        ; outputs: native results; use CY for success/failure.
        ; CY set and A = native error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_f_chdir::
        call    .select_path
        .dw     __zx_esx_gate_a9

        ; __zx_esx_f_mkdir
        ; inputs: A = drive, HL = path.
        ; outputs: native results; use CY for success/failure.
        ; CY set and A = native error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_f_mkdir::
        call    .select_path
        .dw     __zx_esx_gate_aa

        ; __zx_esx_f_rmdir
        ; inputs: A = drive, HL = path.
        ; outputs: native results; use CY for success/failure.
        ; CY set and A = native error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_f_rmdir::
        call    .select_path
        .dw     __zx_esx_gate_ab

        ; __zx_esx_f_stat
        ; inputs: A = drive, HL = path, DE = native status buffer.
        ; outputs: native results; use CY for success/failure.
        ; CY set and A = native error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_f_stat::
        call    .select_path
        .dw     __zx_esx_gate_ac

        ; __zx_esx_f_unlink
        ; inputs: A = drive, HL = path.
        ; outputs: native results; use CY for success/failure.
        ; CY set and A = native error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_f_unlink::
        call    .select_path
        .dw     __zx_esx_gate_ad

        ; __zx_esx_f_rename
        ; inputs: A = drive, HL = old path, DE = new path.
        ; outputs: native results; use CY for success/failure.
        ; CY set and A = native error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_f_rename::
        call    .select_path
        .dw     __zx_esx_gate_b0

        ; __zx_esx_f_opendir
        ; inputs: A = drive, HL = path, B = native directory mode.
        ; outputs: A = native directory handle on success.
        ; CY set and A = native error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_f_opendir::
        call    .select_path
        .dw     __zx_esx_gate_a3

        ; __zx_esx_f_readdir
        ; inputs: A = directory handle, HL = native entry buffer.
        ; outputs: A = nonzero for an entry, zero at end; CY reports error.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_f_readdir::
        call    .select_direct
        .dw     __zx_esx_gate_a4

        ; __zx_esx_f_rewinddir
        ; inputs: A = directory handle; CY reports native error.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_f_rewinddir::
        call    .select_direct
        .dw     __zx_esx_gate_a7

        ; __zx_esx_disk_info
        ; inputs: A = nonzero device id, HL = six-byte result buffer.
        ; outputs: native result; CY reports an unavailable device/error.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_disk_info::
        call    .select_direct
        .dw     __zx_esx_gate_84

        ; __zx_esx_m_execcmd
        ; inputs: HL = NUL-terminated command line in RAM.
        ; outputs: CY clear on success; CY set and A = esxDOS error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_m_execcmd::
        call    .select_direct
        .dw     __zx_esx_gate_8f

        ; Shared direct-RAM call. Caller IY is already stacked.
        ; Consume inline gate address; retain caller IY on stack.
.select_direct:
        .db     0xfd,0xe3               ; EX (SP),IY; explicit prefix for xas sizing
        push    hl
        ld      l,0(iy)
        ld      h,1(iy)
        push    hl
        pop     iy
        pop     hl
.esx_call:
        push    ix
        push    hl
        pop     ix
        call    .esx_path_gate
        push    ix
        pop     hl
        pop     ix
        pop     iy
        ret

        ; .esx_path_call
        ; inputs: A = drive, HL = checked path, DE = second pointer,
        ; B = open mode, IY = RAM gate; caller IY is already stacked.
        ; outputs: native AF/BC/DE/HL results; preserves caller IX/IY.
        ; ROM paths use their length plus NUL in private stack storage,
        ; at most 256 bytes each. RAM paths pass directly to firmware.
        ; Consume inline gate address; retain caller IY on stack.
.select_path:
        .db     0xfd,0xe3               ; EX (SP),IY; explicit prefix for xas sizing
        push    hl
        ld      l,0(iy)
        ld      h,1(iy)
        push    hl
        pop     iy
        pop     hl
.esx_path_call:
        call    __zx_esx_path_drive
        call    __frame_ix
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
        ; divIDE hides the ROM containing the IM2 scheduler. Do not
        ; allow preemption or another firmware call until it unmaps.
        call    _enter_critical_section
        call    .esx_gate_invoke
        jp      _leave_critical_section
.esx_gate_invoke:
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
