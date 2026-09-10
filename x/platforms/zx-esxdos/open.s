        ; Open a headerless esxDOS file with POSIX-style access flags.
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module open
        .optsdcc -mz80 sdcccall(1)

        .globl  _open
        .globl  __zx_esx_path
        .globl  __zx_esx_errno
        .globl  __zx_esx_error
        .globl  __zx_esx_free_fd
        .globl  __zx_esx_f_open
        .globl  __zx_esx_f_stat

        .area   _CODE

        ; inputs: HL = path, DE = flags; output: DE = fd or -1.
        ; clobbers: af, bc, de, hl. IX/IY preserved.
_open::
        call    __zx_esx_path
        jp      c,__zx_esx_errno
        ld      a,e
        and     #0xfc
        jp      nz,.esx_open_invalid
        ld      a,e
        cp      #3
        jp      z,.esx_open_invalid
        ld      a,d
        and     #0xf0
        jp      nz,.esx_open_invalid
        bit     3,d
        jr      z,.esx_open_exclusive_ok
        bit     0,d
        jp      z,.esx_open_invalid
.esx_open_exclusive_ok:
        bit     1,d                     ; O_TRUNC needs write access
        jr      z,.esx_open_flags_ok
        ld      a,e
        or      a
        jp      z,.esx_open_invalid
.esx_open_flags_ok:
        push    ix
        ld      ix,#0
        add     ix,sp
        push    hl                      ; -2: path
        push    de                      ; -4: flags

        ; CREATE_TRUNC also creates missing files. Check existence
        ; first when the caller did not supply O_CREAT.
        bit     1,d
        jr      z,.esx_open_slot
        bit     0,d
        jr      nz,.esx_open_slot
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        ex      de,hl
        ld      l,-2(ix)
        ld      h,-1(ix)
        ld      a,#0x2a
        call    __zx_esx_f_stat
        jp      c,.esx_open_native_error
        push    ix
        pop     hl
        ld      de,#-4
        add     hl,de
        ld      sp,hl
.esx_open_slot:
        call    __zx_esx_free_fd
        jr      c,.esx_open_errno
        push    hl                      ; -6: descriptor entry
        ld      e,a
        ld      d,#0
        push    de                      ; -8: public fd
        ld      a,-4(ix)
        inc     a                       ; access: 0/1/2 -> 1/2/3
        ld      b,a
        ld      a,-3(ix)
        bit     0,a
        jr      z,.esx_open_existing
        bit     3,a                     ; O_CREAT | O_EXCL
        jr      nz,.esx_open_exclusive
        bit     1,a
        jr      nz,.esx_open_truncate
        ld      a,#0x08                 ; existing or create
        jr      .esx_open_mode
.esx_open_exclusive:
        ld      a,#0x04                 ; create new, fail if present
        jr      .esx_open_mode
.esx_open_existing:
        bit     1,a
        jr      nz,.esx_open_truncate
        xor     a
        jr      .esx_open_mode
.esx_open_truncate:
        ld      a,#0x0c
.esx_open_mode:
        or      b
        ld      b,a
        ld      l,-2(ix)
        ld      h,-1(ix)
        ld      a,#0x2a
        call    __zx_esx_f_open
        jr      c,.esx_open_native_error
        ld      l,-6(ix)
        ld      h,-5(ix)
        ld      (hl),a
        inc     hl
        ld      a,-3(ix)
        and     #4                      ; append before every write
        or      -4(ix)                  ; access mode
        or      #0x80                   ; active only after success
        ld      (hl),a
        ld      e,-8(ix)
        ld      d,-7(ix)
        ld      sp,ix
        pop     ix
        ret
.esx_open_native_error:
        call    __zx_esx_error
        jr      .esx_open_return
.esx_open_errno:
        call    __zx_esx_errno
.esx_open_return:
        ld      sp,ix
        pop     ix
        ret
.esx_open_invalid:
        ld      a,#22
        jp      __zx_esx_errno
