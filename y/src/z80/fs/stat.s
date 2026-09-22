        ; POSIX stat over resident esxDOS firmware.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module stat
        .optsdcc -mz80 sdcccall(1)

        .globl  _stat
        .globl  __zx_esx_path
        .globl  __zx_esx_buffer
        .globl  __zx_esx_errno
        .globl  __zx_esx_error
        .globl  __zx_esx_stat_convert
        .globl  __zx_esx_f_stat

        .globl  __frame_ix
        .globl  __frame_return

        .area   _CODE

        ; _stat
        ; inputs: HL = path, DE = struct stat pointer (sdcccall(1)).
        ; outputs: DE = 0 or -1 with errno set.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
_stat::
        call    __zx_esx_path
        jp      c,__zx_esx_errno
        ; The complete 14-byte public object must lie in writable RAM.
        push    hl
        ex      de,hl
        ld      bc,#14
        call    __zx_esx_buffer
        ex      de,hl
        pop     hl
        jp      c,__zx_esx_errno

        call    __frame_ix
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
.return:
        jp      __frame_return
.esx_stat_native_error:
        call    __zx_esx_error
        jr      .return
