        ;; cpmfs_core.s
        ;;
        ;; Common state, public pathname parsing, drive selection, raw-sector
        ;; serialization, descriptor lookup, and cache helpers for libcpmfs.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-08-12   tstih

        .module cpmfs_core
        .optsdcc -mz80 sdcccall(1)

        .include "cpmfs.inc"

        .globl  _cpmfs_init
        .globl  _cpmfs_parse_path
        .globl  _cpmfs_set_drive
        .globl  _cpmfs_get_drive
        .globl  _cpmfs_set_device
        .globl  _cpmfs_get_device
        .globl  _cpmfs_errno
        .globl  __cpmfs_parse_directory
        .globl  __cpmfs_set_error
        .globl  __cpmfs_zero
        .globl  __cpmfs_copy
        .globl  __cpmfs_find_fd
        .globl  __cpmfs_node_dma_iy
        .globl  __cpmfs_read_sector
        .globl  __cpmfs_write_sector
        .globl  __cpmfs_flush_iy
        .globl  __cpmfs_invalidate_iy
        .globl  __cpmfs_entry_matches_iy
        .globl  _memory_init
        .globl  _list_init
        .globl  _list_front
        .globl  _list_next
        .globl  partner_hd_read, partner_hd_write
        .globl  floppy_init
        .globl  floppy_read_sector
        .globl  floppy_write_sector

        .equ    _cpmfs_errno, CPMFS_STATE_ERRNO

        .area   _CODE

        ;; ------------------------------------------------------------
        ;; cpmfs_init
        ;; Initialize shared allocator and filesystem state once.
        ;;
        ;; Signature: int cpmfs_init(void)
        ;;
        ;; Returns: DE = 0 or -1
_cpmfs_init::
        call    _memory_init
        ld      a,d
        and     e
        inc     a
        jr      z,.cpmfs_init_memory_fail
        ld      hl,(CPMFS_STATE_MAGIC)
        ld      de,#CPMFS_MAGIC_VALUE
        or      a
        sbc     hl,de
        jr      z,.cpmfs_init_done
        ld      hl,#CPMFS_MAGIC_VALUE
        ld      (CPMFS_STATE_MAGIC),hl
        ld      hl,#CPMFS_STATE_HEAD
        call    _list_init
        ld      hl,#3
        ld      (CPMFS_STATE_NEXT_FD),hl
        xor     a
        ld      (CPMFS_STATE_DRIVE),a
        ld      (CPMFS_STATE_AREA),a
        ld      (CPMFS_STATE_ERRNO),a
        ld      (CPMFS_STATE_ERRNO + 1),a
        dec     a
        ld      (CPMFS_STATE_ACTIVE_DRIVE),a
.cpmfs_init_done:
        ld      de,#0
        ret
.cpmfs_init_memory_fail:
        ld      a,#CPMFS_ERR_MEMORY
        call    __cpmfs_set_error
        ld      de,#0xffff
        ret

        ;; Recognize an optional FD0:..FD3: or HD0:..HD1: prefix.
        ;; On success CF=1, A=device and HL points just after ':'. When no
        ;; complete prefix is present CF=0 and HL is restored.
.cpmfs_parse_device_hl:
        push    hl
        ld      a,(hl)
        call    .cpmfs_upper_a
        cp      #'F'
        jr      z,.cpmfs_parse_device_fd
        cp      #'H'
        jr      z,.cpmfs_parse_device_hd
        jr      .cpmfs_parse_device_none
.cpmfs_parse_device_fd:
        inc     hl
        ld      a,(hl)
        call    .cpmfs_upper_a
        cp      #'D'
        jr      nz,.cpmfs_parse_device_none
        inc     hl
        ld      a,(hl)
        cp      #'0'
        jr      c,.cpmfs_parse_device_none
        cp      #'0' + CPMFS_FD_DRIVES
        jr      nc,.cpmfs_parse_device_none
        sub     #'0'
        jr      .cpmfs_parse_device_colon
.cpmfs_parse_device_hd:
        inc     hl
        ld      a,(hl)
        call    .cpmfs_upper_a
        cp      #'D'
        jr      nz,.cpmfs_parse_device_none
        inc     hl
        ld      a,(hl)
        cp      #'0'
        jr      c,.cpmfs_parse_device_none
        cp      #'0' + CPMFS_HD_DRIVES
        jr      nc,.cpmfs_parse_device_none
        sub     #'0'
        add     a,#CPMFS_HD_BASE
.cpmfs_parse_device_colon:
        ld      b,a
        inc     hl
        ld      a,(hl)
        cp      #':'
        jr      nz,.cpmfs_parse_device_none
        inc     hl
        ld      a,b
        pop     bc                      ; discard saved original HL
        scf
        ret
.cpmfs_parse_device_none:
        pop     hl
        or      a                       ; clear carry
        ret

        ;; ------------------------------------------------------------
        ;; cpmfs_parse_path
        ;; Parse device:/area/name.ext and default omitted device/area parts.
        ;;
        ;; Signature: int cpmfs_parse_path(pathname, parts)
        ;;
        ;; Arguments: HL = pathname, DE = cpmfs_path_t output
        ;; Returns: DE = 0 or -1
_cpmfs_parse_path::
        push    ix
        push    iy
        push    hl
        push    de
        call    _cpmfs_init
        ld      a,d
        and     e
        inc     a
        pop     de
        pop     hl
        jp      z,.parse_path_init_fail
        ld      a,h
        or      l
        jp      z,.parse_path_fail
        ld      a,d
        or      e
        jp      z,.parse_path_fail
        push    de
        pop     iy
        push    hl
        push    iy
        pop     hl
        ld      b,#PATH_SIZE
        call    __cpmfs_zero
        pop     hl
        ld      a,(CPMFS_STATE_DRIVE)
        ld      PATH_DRIVE(iy),a
        ld      a,(CPMFS_STATE_AREA)
        ld      PATH_AREA(iy),a
        push    hl
        push    iy
        pop     hl
        ld      de,#PATH_CANONICAL
        add     hl,de
        ld      b,#11
        ld      a,#' '
.parse_path_space_loop:
        ld      (hl),a
        inc     hl
        djnz    .parse_path_space_loop
        pop     hl

        ld      a,(hl)
        or      a
        jp      z,.parse_path_fail
        call    .cpmfs_parse_device_hl
        jr      nc,.parse_path_after_drive
        ld      PATH_DRIVE(iy),a
.parse_path_after_drive:
        ld      a,(hl)
        cp      #'/'
        jr      z,.parse_path_skip_separator
        jr      .parse_path_try_area
.parse_path_skip_separator:
        inc     hl

.parse_path_try_area:
        push    hl
        ld      b,#0
        ld      c,#0
.parse_path_area_digits:
        ld      a,(hl)
        cp      #'0'
        jr      c,.parse_path_area_end
        cp      #'9' + 1
        jr      nc,.parse_path_area_end
        inc     c
        ld      a,c
        cp      #3
        jr      nc,.parse_path_area_not_prefix
        ld      a,b
        add     a,a
        ld      d,a
        add     a,a
        add     a,a
        add     a,d
        ld      b,a
        ld      a,(hl)
        sub     #'0'
        add     a,b
        ld      b,a
        inc     hl
        jr      .parse_path_area_digits
.parse_path_area_end:
        ld      a,c
        or      a
        jr      z,.parse_path_area_not_prefix
        ld      a,(hl)
        cp      #'/'
        jr      z,.parse_path_area_prefix
        jr      .parse_path_area_not_prefix
.parse_path_area_prefix:
        ld      a,b
        cp      #CPMFS_AREAS
        jp      nc,.parse_path_fail_pop
        ld      PATH_AREA(iy),a
        pop     de
        inc     hl
        jr      .parse_path_name
.parse_path_area_not_prefix:
        pop     hl

.parse_path_name:
        push    iy
        pop     de
        ld      bc,#PATH_FILENAME
        ex      de,hl
        add     hl,bc
        ex      de,hl                  ; DE = filename output
        push    iy
        pop     bc
        ld      a,c
        add     a,#PATH_CANONICAL
        ld      c,a
        ld      a,b
        adc     a,#0
        ld      b,a
        ld      (CPMFS_TMP_POINTER),bc
        ld      c,#0
.parse_path_name_loop:
        ld      a,(hl)
        or      a
        jr      z,.parse_path_name_done
        cp      #'.'
        jr      z,.parse_path_extension_start
        cp      #'/'
        jp      z,.parse_path_fail
        cp      #0x5c
        jp      z,.parse_path_fail
        call    .cpmfs_upper_a
        call    .cpmfs_is_name_char
        jp      nc,.parse_path_fail
        ld      a,c
        cp      #CPMFS_FILENAME_MAX
        jp      nc,.parse_path_fail
        ld      a,(hl)
        call    .cpmfs_upper_a
        ld      (de),a
        inc     de
        push    hl
        ld      hl,(CPMFS_TMP_POINTER)
        ld      (hl),a
        inc     hl
        ld      (CPMFS_TMP_POINTER),hl
        pop     hl
        inc     hl
        inc     c
        jr      .parse_path_name_loop
.parse_path_name_done:
        ld      a,c
        or      a
        jp      z,.parse_path_fail
        jr      .parse_path_success

.parse_path_extension_start:
        ld      a,c
        or      a
        jp      z,.parse_path_fail
        inc     hl
        push    iy
        pop     de
        ld      bc,#PATH_EXTENSION
        ex      de,hl
        add     hl,bc
        ex      de,hl
        push    iy
        pop     bc
        ld      a,c
        add     a,#(PATH_CANONICAL + 8)
        ld      c,a
        ld      a,b
        adc     a,#0
        ld      b,a
        ld      (CPMFS_TMP_POINTER),bc
        ld      c,#0
.parse_path_extension_loop:
        ld      a,(hl)
        or      a
        jr      z,.parse_path_extension_done
        cp      #'.'
        jp      z,.parse_path_fail
        cp      #'/'
        jp      z,.parse_path_fail
        cp      #0x5c
        jp      z,.parse_path_fail
        call    .cpmfs_upper_a
        call    .cpmfs_is_name_char
        jp      nc,.parse_path_fail
        ld      a,c
        cp      #CPMFS_EXTENSION_MAX
        jp      nc,.parse_path_fail
        ld      a,(hl)
        call    .cpmfs_upper_a
        ld      (de),a
        inc     de
        push    hl
        ld      hl,(CPMFS_TMP_POINTER)
        ld      (hl),a
        inc     hl
        ld      (CPMFS_TMP_POINTER),hl
        pop     hl
        inc     hl
        inc     c
        jr      .parse_path_extension_loop
.parse_path_extension_done:
        ld      a,c
        or      a
        jp      z,.parse_path_fail
.parse_path_success:
        xor     a
        call    __cpmfs_set_error
        ld      de,#0
        pop     iy
        pop     ix
        ret
.parse_path_fail_pop:
        pop     hl
.parse_path_fail:
        ld      a,#CPMFS_ERR_PATH
        call    __cpmfs_set_error
.parse_path_init_fail:
        ld      de,#0xffff
        pop     iy
        pop     ix
        ret

        ;; ------------------------------------------------------------
        ;; __cpmfs_parse_directory
        ;; Parse an optional fdN:/hdN: device plus a numeric user-area path.
        ;;
        ;; Arguments: HL = path
        ;; Returns: A=0, B=device, C=area; A=1 on failure
__cpmfs_parse_directory::
        ld      a,h
        or      l
        jp      z,.parse_directory_fail
        ld      b,#0
        ld      a,(CPMFS_STATE_DRIVE)
        ld      d,a
        ld      a,(CPMFS_STATE_AREA)
        ld      e,a
        ld      a,(hl)
        or      a
        jp      z,.parse_directory_defaults
        cp      #'.'
        jr      nz,.parse_directory_drive
        inc     hl
        ld      a,(hl)
        or      a
        jp      z,.parse_directory_defaults
        jp      .parse_directory_fail
.parse_directory_drive:
        call    .cpmfs_parse_device_hl
        jr      nc,.parse_directory_after_drive
        ld      d,a
.parse_directory_after_drive:
        ld      a,(hl)
        cp      #'/'
        jr      z,.parse_directory_separator
        jr      .parse_directory_digits
.parse_directory_separator:
        inc     hl
        ld      a,(hl)
        or      a
        jp      nz,.parse_directory_digits
        ld      e,#0                    ; root convention is user area zero
        jp      .parse_directory_defaults
.parse_directory_digits:
        ld      b,#0
        ld      c,#0
.parse_directory_digit_loop:
        ld      a,(hl)
        cp      #'0'
        jr      c,.parse_directory_digit_end
        cp      #'9' + 1
        jr      nc,.parse_directory_digit_end
        inc     c
        ld      a,c
        cp      #3
        jp      nc,.parse_directory_fail
        ld      a,b
        add     a,a
        ld      e,a
        add     a,a
        add     a,a
        add     a,e
        ld      b,a
        ld      a,(hl)
        sub     #'0'
        add     a,b
        ld      b,a
        inc     hl
        jr      .parse_directory_digit_loop
.parse_directory_digit_end:
        ld      a,c
        or      a
        jp      z,.parse_directory_fail
        ld      a,b
        cp      #CPMFS_AREAS
        jp      nc,.parse_directory_fail
        ld      e,a
        ld      a,(hl)
        or      a
        jp      z,.parse_directory_defaults
        cp      #'/'
        jr      z,.parse_directory_trailing
        jp      .parse_directory_fail
.parse_directory_trailing:
        inc     hl
        ld      a,(hl)
        or      a
        jp      nz,.parse_directory_fail
.parse_directory_defaults:
        ld      b,d
        ld      c,e
        xor     a
        ret
.parse_directory_fail:
        ld      a,#1
        or      a
        ret

        ;; ------------------------------------------------------------
        ;; cpmfs_set_device / cpmfs_get_device
_cpmfs_set_device::
_cpmfs_set_drive::
        push    af
        call    _cpmfs_init
        ld      a,d
        and     e
        inc     a
        jr      z,.cpmfs_set_drive_init_fail
        pop     af
        cp      #CPMFS_DEVICES
        jr      nc,.cpmfs_set_drive_fail
        ld      (CPMFS_STATE_DRIVE),a
        xor     a
        call    __cpmfs_set_error
        ld      de,#0
        ret
.cpmfs_set_drive_fail:
        ld      a,#CPMFS_ERR_ARGUMENT
        call    __cpmfs_set_error
        ld      de,#0xffff
        ret
.cpmfs_set_drive_init_fail:
        pop     af
        ld      de,#0xffff
        ret

_cpmfs_get_device::
_cpmfs_get_drive::
        call    _cpmfs_init
        ld      a,(CPMFS_STATE_DRIVE)
        ret

        ;; Store A as a 16-bit library diagnostic.
__cpmfs_set_error::
        ld      (CPMFS_STATE_ERRNO),a
        xor     a
        ld      (CPMFS_STATE_ERRNO + 1),a
        ret

        ;; Zero B bytes at HL.
__cpmfs_zero::
        xor     a
.cpmfs_zero_loop:
        ld      (hl),a
        inc     hl
        djnz    .cpmfs_zero_loop
        ret

        ;; Copy B bytes from HL to DE.
__cpmfs_copy::
        ld      a,(hl)
        ld      (de),a
        inc     hl
        inc     de
        djnz    __cpmfs_copy
        ret

.cpmfs_upper_a:
        cp      #'a'
        ret     c
        cp      #'z' + 1
        ret     nc
        sub     #32
        ret

        ;; Validate one normalized CP/M filename character; carry means valid.
.cpmfs_is_name_char:
        cp      #0x21
        jr      c,.cpmfs_name_invalid
        cp      #0x7f
        jr      nc,.cpmfs_name_invalid
        cp      #'<'
        jr      z,.cpmfs_name_invalid
        cp      #'>'
        jr      z,.cpmfs_name_invalid
        cp      #','
        jr      z,.cpmfs_name_invalid
        cp      #';'
        jr      z,.cpmfs_name_invalid
        cp      #':'
        jr      z,.cpmfs_name_invalid
        cp      #'='
        jr      z,.cpmfs_name_invalid
        cp      #'?'
        jr      z,.cpmfs_name_invalid
        cp      #'*'
        jr      z,.cpmfs_name_invalid
        cp      #'['
        jr      z,.cpmfs_name_invalid
        cp      #']'
        jr      z,.cpmfs_name_invalid
        cp      #'/'
        jr      z,.cpmfs_name_invalid
        cp      #0x5c
        jr      z,.cpmfs_name_invalid
        cp      #'|'
        jr      z,.cpmfs_name_invalid
        scf
        ret
.cpmfs_name_invalid:
        or      a
        ret

        ;; ------------------------------------------------------------
        ;; __cpmfs_find_fd
        ;; Locate one intrusive file node by descriptor number.
        ;;
        ;; Arguments: HL = fd
        ;; Returns: HL = node or zero
__cpmfs_find_fd::
        ld      b,h
        ld      c,l
        ld      hl,#CPMFS_STATE_HEAD
        call    _list_front
        ex      de,hl
.cpmfs_find_fd_loop:
        ld      a,h
        or      l
        jr      z,.cpmfs_find_fd_fail
        push    hl
        inc     hl
        inc     hl
        ld      a,(hl)
        cp      c
        jr      nz,.cpmfs_find_fd_next
        inc     hl
        ld      a,(hl)
        cp      b
        jr      z,.cpmfs_find_fd_hit
.cpmfs_find_fd_next:
        pop     hl
        call    _list_next
        ex      de,hl
        jr      .cpmfs_find_fd_loop
.cpmfs_find_fd_hit:
        pop     hl
        ret
.cpmfs_find_fd_fail:
        ld      hl,#0
        ret

        ;; Return HL = inline DMA buffer for IY file node.
__cpmfs_node_dma_iy::
        push    de
        push    iy
        pop     hl
        ld      de,#FILE_DMA
        add     hl,de
        pop     de
        ret

        ;; Select a physical drive only when it differs from the active one.
.cpmfs_select_drive:
        push    af
        ld      a,(CPMFS_STATE_ACTIVE_DRIVE)
        ld      b,a
        pop     af
        cp      b
        jr      z,.cpmfs_select_drive_done
        push    af
        call    floppy_init
        pop     bc                       ; B = requested physical drive
        or      a
        ret     nz
        ld      a,b
        ld      (CPMFS_STATE_ACTIVE_DRIVE),a
.cpmfs_select_drive_done:
        xor     a
        ret

        ;; A=drive, DE=LBA, HL=buffer. Return libfloppy status in A.
__cpmfs_read_sector::
        cp      #CPMFS_FD_DRIVES
        jr      nc,.cpmfs_hd_read
        push    de
        push    hl
        call    .cpmfs_select_drive
        pop     hl
        pop     de
        or      a
        ret     nz
        jp      floppy_read_sector

        ;; A=drive, DE=LBA, HL=buffer. Return libfloppy status in A.
__cpmfs_write_sector::
        cp      #CPMFS_FD_DRIVES
        jr      nc,.cpmfs_hd_write
        push    de
        push    hl
        call    .cpmfs_select_drive
        pop     hl
        pop     de
        or      a
        ret     nz
        jp      floppy_write_sector
.cpmfs_hd_read:
        sub     #4
        or      a
        jr      nz,.cpmfs_sector_device_fail
        jp      partner_hd_read
.cpmfs_hd_write:
        sub     #4
        or      a
        jr      nz,.cpmfs_sector_device_fail
        jp      partner_hd_write
.cpmfs_sector_device_fail:
        ld      a,#1
        or      a
        ret

        ;; Flush a dirty file cache. IY=node, A=0 on success.
__cpmfs_flush_iy::
        ld      a,FILE_CACHE_DIRTY(iy)
        or      a
        ret     z
        ld      e,FILE_CACHE_LBA(iy)
        ld      d,FILE_CACHE_LBA + 1(iy)
        call    __cpmfs_node_dma_iy
        ld      a,FILE_DRIVE(iy)
        call    __cpmfs_write_sector
        or      a
        ret     nz
        xor     a
        ld      FILE_CACHE_DIRTY(iy),a
        ret

        ;; Flush and invalidate a file cache. IY=node, A=0 on success.
__cpmfs_invalidate_iy::
        call    __cpmfs_flush_iy
        ret     nz
        xor     a
        ld      FILE_CACHE_VALID(iy),a
        ret

        ;; Compare entry at HL with IY node area/name. Z means match.
__cpmfs_entry_matches_iy::
        ld      a,(hl)
        cp      FILE_AREA(iy)
        ret     nz
        push    hl
        inc     hl
        push    iy
        pop     de
        ld      bc,#FILE_NAME
        ex      de,hl
        add     hl,bc
        ex      de,hl
        ld      b,#11
.cpmfs_entry_name_loop:
        ld      a,(hl)
        and     #0x7f
        ld      c,a
        ld      a,(de)
        cp      c
        jr      nz,.cpmfs_entry_name_fail
        inc     hl
        inc     de
        djnz    .cpmfs_entry_name_loop
        pop     hl
        xor     a
        ret
.cpmfs_entry_name_fail:
        pop     hl
        ld      a,#1
        or      a
        ret
