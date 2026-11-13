        ; Open an native directory as an opaque POSIX-style DIR object.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module opendir
        .optsdcc -mz80 sdcccall(1)

        .globl  _opendir
        .globl  __near_malloc
        .globl  __near_free
        .globl  __os_malloc
        .globl  __next_fs_path
        .globl  __next_fs_errno
        .globl  __next_fs_error
        .globl  __next_fs_f_opendir

        .equ    DIRECTORY_MAGIC, 0xd1
        .equ    DIRECTORY_SIZE,  303

        .globl  __frame_ix
        .globl  __frame_return

        .area   _CODE

        ; input: HL = path; output: DE = DIR pointer or NULL.
_opendir::
        call    __next_fs_path
        jr      nc,.path_ok
        call    __next_fs_errno
        ld      de,#0
        ret
.path_ok:
        call    __frame_ix
        push    hl                      ; IX-2: path
        ld      hl,#DIRECTORY_SIZE
        call    __near_malloc
        ld      a,d
        or      e
        jr      nz,.allocated
        ; A transactional navigation keeps the old DIR alive until the new
        ; open succeeds. Tight application banks may not hold two 303-byte
        ; records; fixed memory is also a stable near address and near_free
        ; already releases either allocation kind.
        ld      hl,#DIRECTORY_SIZE
        call    __os_malloc
        ld      a,d
        or      e
        jr      z,.no_memory
.allocated:
        push    de                      ; IX-4: directory
        ld      l,-2(ix)
        ld      h,-1(ix)
        ld      a,#0x2a                 ; current drive
        ld      b,#0                    ; short names, no BASIC header
        call    __next_fs_f_opendir
        jr      c,.native_error
        ld      l,-4(ix)
        ld      h,-3(ix)
        ld      (hl),a                  ; native handle
        inc     hl
        ld      (hl),#DIRECTORY_MAGIC
        ld      e,-4(ix)
        ld      d,-3(ix)
        jp      __frame_return
.native_error:
        push    af
        ld      l,-4(ix)
        ld      h,-3(ix)
        call    __near_free
        pop     af
        call    __next_fs_error
        jr      .null
.no_memory:
        ld      a,#12                   ; ENOMEM
        call    __next_fs_errno
.null:
        ld      de,#0
        jp      __frame_return
