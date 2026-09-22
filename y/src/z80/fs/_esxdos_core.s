        ; esxDOS 0.8.9 descriptor state and checked ABI helpers.
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module esxdos_core
        .optsdcc -mz80 sdcccall(1)

        .globl  __errno_value
        .globl  __zx_esx_files
        .globl  __zx_esx_errno
        .globl  __zx_esx_error
        .globl  __zx_esx_path
        .globl  __zx_esx_buffer
        .globl  __zx_esx_source
        .globl  __zx_esx_fd
        .globl  __zx_esx_free_fd
        .globl  __zx_esx_stat_convert

        .area   _CODE

        ; A = POSIX errno. Both word and long failure results are -1.

        ; __zx_esx_errno
        ; inputs: A = POSIX errno value.
        ; outputs: errno set, HL:DE = -1.
        ; clobbers: de, hl; preserves a, bc, ix and iy.
__zx_esx_errno::
        ld      l,a
        ld      h,#0
        ld      (__errno_value),hl
        ld      hl,#0xffff
        ld      d,h
        ld      e,l
        ret

        ; Native esxDOS errors are not the libc errno numbers.

        ; __zx_esx_error
        ; inputs: A = native esxDOS error.
        ; outputs: errno translated, HL:DE = -1.
        ; clobbers: af, de, hl; preserves bc, ix and iy.
__zx_esx_error::
        cp      #32
        jr      nc,.esx_unknown_error
        ld      e,a
        ld      d,#0
        ld      hl,#.esx_errors
        add     hl,de
        ld      a,(hl)
.set_errno:
        jp      __zx_esx_errno
.esx_unknown_error:
        ld      a,#5                    ; EIO
        jr      .set_errno
.esx_errors:
        .db     5,5,22,22,22,2,5,22
        .db     13,28,6,19,24,9,19,75
        .db     21,20,17,2,38,36,2,16
        .db     30,5,5,39,16,16,19,16

        ; HL = fd. On success HL = entry, A = handle, C = flags.
        ; B, DE, IX and IY preserved. Failure: CY, A = EBADF.

        ; __zx_esx_fd
        ; inputs: HL = public file descriptor.
        ; outputs: HL = entry, A = native handle, C = flags; CY clear.
        ; CY set and A = EBADF on failure.
        ; clobbers: af, c, hl; preserves b, de, ix and iy.
__zx_esx_fd::
        ld      a,h
        or      a
        jr      nz,.esx_bad_fd
        ld      a,l
        sub     #3
        cp      #16
        jr      nc,.esx_bad_fd
        push    de
        ld      e,a
        ld      d,#0
        ld      hl,#__zx_esx_files
        add     hl,de
        add     hl,de
        pop     de
        inc     hl
        ld      c,(hl)
        dec     hl
        bit     7,c
        jr      z,.esx_bad_fd
        ld      a,(hl)
        or      a
        ret
.esx_bad_fd:
        ld      a,#9
        scf
        ret

        ; Return HL = inactive entry, A = fd, carry clear; else EMFILE.

        ; __zx_esx_free_fd
        ; inputs: none.
        ; outputs: HL = free entry, A = fd, CY clear; else A = EMFILE
        ; and CY set.
        ; clobbers: af, bc, hl; preserves de, ix and iy.
__zx_esx_free_fd::
        ld      hl,#__zx_esx_files + 1
        ld      b,#16
        ld      c,#3
.esx_free_loop:
        bit     7,(hl)
        jr      z,.esx_free_found
        inc     hl
        inc     hl
        inc     c
        djnz    .esx_free_loop
        ld      a,#24
        scf
        ret
.esx_free_found:
        dec     hl
        ld      a,c
        or      a
        ret

        ; Validate a NUL-terminated path, 1..255 bytes. ROM paths are
        ; copied to the stack by the native-call wrappers before paging.
        ; Carry set and A = errno on failure.

        ; __zx_esx_path
        ; inputs: HL = NUL-terminated ROM or RAM path.
        ; outputs: CY clear if valid; CY set and A = errno otherwise.
        ; clobbers: af; preserves bc, de, hl, ix and iy.
__zx_esx_path::
        push    hl
        push    bc
        ld      a,h
        or      l
        jr      z,.esx_path_fault
        ld      a,(hl)
        or      a
        jr      z,.esx_path_empty
        ld      b,#0
.esx_path_loop:
        ld      a,(hl)
        or      a
        jr      z,.esx_path_ok
        inc     hl
        ld      a,h
        or      l
        jr      z,.esx_path_fault
        djnz    .esx_path_loop
        ld      a,#36                   ; ENAMETOOLONG
        jr      .esx_path_error
.esx_path_fault:
        ld      a,#14
        jr      .esx_path_error
.esx_path_empty:
        ld      a,#2
.esx_path_error:
        scf
        pop     bc
        pop     hl
        ret
.esx_path_ok:
        pop     bc
        pop     hl
        ret

        ; HL = RAM buffer, BC = nonnegative length. Zero length ignores
        ; HL.
        ; Preserve all argument registers. A/CY report EFAULT on bad
        ; span.

        ; __zx_esx_buffer
        ; inputs: HL = RAM buffer, BC = length.
        ; outputs: CY clear if valid; CY set and A = EFAULT otherwise.
        ; clobbers: af; preserves bc, de, hl, ix and iy.
__zx_esx_buffer::
        ld      a,b
        or      c
        ret     z
        ld      a,h
        cp      #0x40
        jr      c,.esx_buffer_bad
.esx_span:
        push    hl
        dec     bc
        add     hl,bc                   ; last byte, not one-past
        inc     bc
        pop     hl
        ret     nc
.esx_buffer_bad:
        ld      a,#14
        scf
        ret

        ; __zx_esx_source
        ; inputs: HL = readable ROM/RAM buffer, BC = length.
        ; outputs: CY clear if valid; CY set and A = EFAULT otherwise.
        ; clobbers: af; preserves bc, de, hl, ix and iy.
        ; A zero length ignores HL. Nonempty spans must not wrap.
__zx_esx_source::
        ld      a,b
        or      c
        ret     z
        ld      a,h
        or      l
        jr      z,.esx_buffer_bad
        jr      .esx_span

        ; HL = 11-byte firmware stat, DE = validated 14-byte struct
        ; stat.
        ; A signed off_t cannot describe files larger than LONG_MAX.

        ; __zx_esx_stat_convert
        ; inputs: HL = native 11-byte status, DE = public 14-byte
        ; struct stat in validated RAM.
        ; outputs: DE = 0 or -1 with errno set.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__zx_esx_stat_convert::
        push    hl
        push    de
        inc     hl
        inc     hl
        ld      a,(hl)                  ; native attributes
        ; Directories report 0xffffffff as a size sentinel in 0.8.9.
        ; They have no byte-stream length; expose st_size = 0.
        bit     4,a
        jr      nz,.esx_stat_valid
        ld      bc,#8
        add     hl,bc
        bit     7,(hl)
        jr      nz,.esx_stat_overflow
.esx_stat_valid:
        pop     de
        pop     hl
        ldi                             ; device identifier
        ldi
        push    hl                      ; native attributes address
        ld      c,a
        xor     a
        ld      b,#12
.esx_stat_clear:
        ld      (de),a
        inc     de
        djnz    .esx_stat_clear
        ex      de,hl                   ; public status + 14
        ld      de,#-8
        add     hl,de                   ; st_mode at +6
        ld      de,#0x8124              ; S_IFREG | 0444
        bit     0,c
        jr      nz,.esx_stat_permissions
        ld      e,#0xb6                 ; 0666
.esx_stat_permissions:
        bit     4,c
        jr      z,.esx_stat_mode
        ld      d,#0x41                 ; S_IFDIR
        ld      a,e
        or      #0x49                   ; add 0111 for traversal
        ld      e,a
.esx_stat_mode:
        ld      (hl),e
        inc     hl
        ld      (hl),d
        inc     hl
        ld      (hl),#1                 ; st_nlink
        inc     hl
        inc     hl                      ; st_size at +10
        ex      de,hl
        pop     hl                      ; native status +2
        bit     4,c
        jr      nz,.esx_stat_done
        ld      bc,#5
        add     hl,bc                   ; native size at +7
        ld      bc,#4
        ldir
.esx_stat_done:
        ld      de,#0
        ret
.esx_stat_overflow:
        pop     de
        pop     hl
        ld      a,#75
        jp      __zx_esx_errno

        ; Platform descriptor state, not libc allocation or shared
        ; scratch.
        ; Each entry is native handle, active/access/append flags.

        .area   _BSS
__zx_esx_files:
        .ds     32
