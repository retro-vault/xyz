        ; Open an esxDOS directory as an opaque POSIX-style DIR object.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module opendir
        .optsdcc -mz80 sdcccall(1)

        .globl  _opendir
        .globl  __os_malloc
        .globl  __os_free
        .globl  __zx_esx_path
        .globl  __zx_esx_errno
        .globl  __zx_esx_error
        .globl  __zx_esx_f_opendir

        .equ    DIRECTORY_MAGIC, 0xd1
        .equ    DIRECTORY_SIZE,  47

        .globl  __frame_ix
        .globl  __frame_return

        .area   _CODE

        ; input: HL = path; output: DE = DIR pointer or NULL.
_opendir::
        call    __zx_esx_path
        jr      nc,.path_ok
        call    __zx_esx_errno
        ld      de,#0
        ret
.path_ok:
        call    __frame_ix
        push    hl                      ; IX-2: path
        ld      hl,#DIRECTORY_SIZE
        call    __os_malloc
        ld      a,d
        or      e
        jr      z,.no_memory
        push    de                      ; IX-4: directory
        ld      l,-2(ix)
        ld      h,-1(ix)
        ld      a,#0x2a                 ; current drive
        ld      b,#0                    ; short names, no BASIC header
        call    __zx_esx_f_opendir
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
        call    __os_free
        pop     af
        call    __zx_esx_error
        jr      .null
.no_memory:
        ld      a,#12                   ; ENOMEM
        call    __zx_esx_errno
.null:
        ld      de,#0
        jp      __frame_return
