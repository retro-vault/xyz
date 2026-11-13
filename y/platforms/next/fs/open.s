        ; Open a headerless native filesystem file with POSIX-style access flags.
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module open
        .optsdcc -mz80 sdcccall(1)

        .globl  _open
        .globl  __critical_call
        .globl  __next_fs_path
        .globl  __next_fs_errno
        .globl  __next_fs_error
        .globl  __next_fs_free_fd
        .globl  __next_fs_f_open
        .globl  __next_fs_f_stat

        .globl  __frame_ix
        .globl  __frame_return

        .area   _CODE

        ; inputs: HL = path, DE = flags; output: DE = fd or -1.
        ; clobbers: af, bc, de, hl. IX/IY preserved.
_open::
        call    __critical_call
        call    __next_fs_path
        jp      c,__next_fs_errno
        ld      a,e
        and     #0xfc
        jp      nz,.native_open_invalid
        ld      a,e
        cp      #3
        jp      z,.native_open_invalid
        ld      a,d
        and     #0xf0
        jp      nz,.native_open_invalid
        bit     3,d
        jr      z,.native_open_exclusive_ok
        bit     0,d
        jp      z,.native_open_invalid
.native_open_exclusive_ok:
        bit     1,d                     ; O_TRUNC needs write access
        jr      z,.native_open_flags_ok
        ld      a,e
        or      a
        jp      z,.native_open_invalid
.native_open_flags_ok:
        call    __frame_ix
        push    hl                      ; -2: path
        push    de                      ; -4: flags

        ; CREATE_TRUNC also creates missing files. Check existence
        ; first when the caller did not supply O_CREAT.
        bit     1,d
        jr      z,.native_open_slot
        bit     0,d
        jr      nz,.native_open_slot
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        ex      de,hl
        ld      l,-2(ix)
        ld      h,-1(ix)
        ld      a,#0x2a
        call    __next_fs_f_stat
        jr      c,.native_open_native_error
        push    ix
        pop     hl
        ld      de,#-4
        add     hl,de
        ld      sp,hl
.native_open_slot:
        call    __next_fs_free_fd
        jr      c,.native_open_errno
        push    hl                      ; -6: descriptor entry
        ld      e,a
        ld      d,#0
        push    de                      ; -8: public fd
        ld      a,-4(ix)
        inc     a                       ; access: 0/1/2 -> 1/2/3
        ld      b,a
        ld      a,-3(ix)
        bit     0,a
        jr      z,.native_open_existing
        bit     3,a                     ; O_CREAT | O_EXCL
        jr      nz,.native_open_exclusive
        bit     1,a
        jr      nz,.native_open_truncate
        ld      a,#0x08                 ; existing or create
        jr      .native_open_mode
.native_open_exclusive:
        ld      a,#0x04                 ; create new, fail if present
        jr      .native_open_mode
.native_open_existing:
        bit     1,a
        jr      nz,.native_open_truncate
        xor     a
        jr      .native_open_mode
.native_open_truncate:
        ld      a,#0x0c
.native_open_mode:
        or      b
        ld      b,a
        ld      l,-2(ix)
        ld      h,-1(ix)
        ld      a,#0x2a
        call    __next_fs_f_open
        jr      c,.native_open_native_error
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
        jp      __frame_return
.native_open_native_error:
        call    __next_fs_error
        jr      .native_open_return
.native_open_errno:
        call    __next_fs_errno
.native_open_return:
        jp      __frame_return
.native_open_invalid:
        ld      a,#22
        jp      __next_fs_errno
