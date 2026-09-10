        ; fstat.s -- POSIX status for console and esxDOS descriptors
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fstat
        .optsdcc -mz80 sdcccall(1)

        .globl  _fstat
        .globl  __zx_esx_fd
        .globl  __zx_esx_buffer
        .globl  __zx_esx_errno
        .globl  __zx_esx_error
        .globl  __zx_esx_stat_convert
        .globl  __zx_esx_f_fstat

        .area   _CODE

        ; _fstat
        ; inputs: HL = fd, DE = struct stat pointer (sdcccall(1)).
        ; outputs: DE = 0 or -1 with errno set.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
_fstat::
        push    ix
        ld      ix,#0
        add     ix,sp
        ; IX-2: 14-byte public status pointer
        push    de
        push    hl                      ; IX-4: descriptor
        ex      de,hl
        ld      bc,#14
        call    __zx_esx_buffer
        jp      c,.esx_fstat_errno
        ld      l,-4(ix)
        ld      h,-3(ix)
        ld      a,h
        or      a
        jr      nz,.esx_fstat_file
        ld      a,l
        cp      #3
        jr      c,.esx_fstat_console
.esx_fstat_file:
        call    __zx_esx_fd
        jr      c,.esx_fstat_errno
        push    af                      ; IX-5: native handle
        ld      hl,#-12
        add     hl,sp
        ; IX-18: raw 11-byte status, pad byte
        ld      sp,hl
        ld      a,-5(ix)
        call    __zx_esx_f_fstat
        jr      c,.esx_fstat_native_error
        push    ix
        pop     hl
        ld      bc,#-18
        add     hl,bc
        ld      e,-2(ix)
        ld      d,-1(ix)
        call    __zx_esx_stat_convert
        ld      sp,ix
        pop     ix
        ret

.esx_fstat_console:
        ld      e,-2(ix)
        ld      d,-1(ix)
        xor     a
        ld      b,#14
.esx_fstat_clear_console:
        ld      (de),a
        inc     de
        djnz    .esx_fstat_clear_console
        ld      l,-2(ix)
        ld      h,-1(ix)
        ld      de,#6
        add     hl,de
        ld      (hl),#0xb6              ; S_IFCHR | 0666 = 0x21b6
        inc     hl
        ld      (hl),#0x21
        inc     hl
        ld      (hl),#1                 ; st_nlink = 1
        ld      de,#0
        ld      sp,ix
        pop     ix
        ret

.esx_fstat_errno:
        call    __zx_esx_errno
        ld      sp,ix
        pop     ix
        ret
.esx_fstat_native_error:
        call    __zx_esx_error
        ld      sp,ix
        pop     ix
        ret
