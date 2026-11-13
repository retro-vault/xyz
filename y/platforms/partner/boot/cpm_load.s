        ;; cpm_load.s
        ;;
        ;; Private first-stage CP/M 3 whole-file reader for a classic Partner P
        ;; floppy. It supports user areas, 8.3 names, sequential logical
        ;; extents, and eight 16-bit allocation pointers per extent.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-08-10   tstih

        .module loader_cpm

        .include "floppy.inc"
        .include "cpm_load.inc"

        .globl  loader_cpm_load
        .globl  floppy_read_sector

        .area   _CODE

        ;; ------------------------------------------------------------
        ;; loader_cpm_load
        ;; Load a CP/M file from the Partner P disk. The expected size is part
        ;; of the boot manifest and the host image builder pads the CP/M file
        ;; to a whole physical sector. Directory RC fields are checked against
        ;; that size so replacing the file without regenerating the manifest
        ;; fails instead of transferring an arbitrary prefix.
        ;;
        ;; Arguments:
        ;;   A  = CP/M user area (0..15)
        ;;   B  = expected number of 256-byte sectors (1..255)
        ;;   DE = page-aligned destination address
        ;;   HL = canonical, space-padded 8+3 filename (11 bytes)
        ;;
        ;; Returns: A = LOADER_CPM_OK/Z or error/NZ; interrupts enabled
        ;;
        ;; Clobbers: AF, BC, DE, HL, IX
loader_cpm_load::
        cp      #16
        jp      nc,loader_cpm_argument_error
        ld      (loader_cpm_user),a
        ld      a,b
        or      a
        jp      z,loader_cpm_argument_error
        ld      (loader_cpm_sectors_left),a
        ld      a,e
        or      a
        jp      nz,loader_cpm_argument_error
        ld      (loader_cpm_destination),de
        ld      (loader_cpm_name),hl
        xor     a
        ld      (loader_cpm_extent),a

.loader_cpm_next_extent:
        call    loader_cpm_find_extent
        ret     nz
        call    loader_cpm_validate_extent
        jp      nz,loader_cpm_corrupt_error
        call    loader_cpm_load_extent
        ret     nz
        ld      a,(loader_cpm_sectors_left)
        or      a
        jr      z,.loader_cpm_check_end
        ld      hl,#loader_cpm_extent
        inc     (hl)
        jr      .loader_cpm_next_extent

.loader_cpm_check_end:
        ;; A final RC check catches most size mismatches. Also reject a following
        ;; extent so a manifest ending exactly on a 16 KiB boundary cannot load
        ;; only the prefix of a larger replacement file.
        ld      hl,#loader_cpm_extent
        inc     (hl)
        call    loader_cpm_find_extent
        cp      #LOADER_CPM_ERR_NOT_FOUND
        jr      z,.loader_cpm_load_done
        or      a
        ret     nz                      ; propagate a directory read failure
        jp      loader_cpm_corrupt_error     ; an additional extent exists

.loader_cpm_load_done:
        xor     a
        ret

        ;; ------------------------------------------------------------
        ;; loader_cpm_find_extent
        ;; Scan all 128 directory entries for the requested user/name/extent.
        ;;
        ;; Returns: IX = matching entry and A=0/Z, or A=error/NZ
        ;;
        ;; Clobbers: AF, BC, DE, HL, IX
loader_cpm_find_extent:
        ld      hl,#LOADER_CPM_DIRECTORY_LBA
        ld      (loader_cpm_directory_lba),hl
        ld      a,#LOADER_CPM_DIRECTORY_SECTORS
        ld      (loader_cpm_directory_left),a

.loader_cpm_find_sector:
        ld      de,(loader_cpm_directory_lba)
        ld      hl,#LOADER_CPM_DIRECTORY_BUFFER
        call    floppy_read_sector
        jr      nz,.loader_cpm_find_floppy_error
        ld      ix,#LOADER_CPM_DIRECTORY_BUFFER
        ld      b,#LOADER_CPM_ENTRIES_PER_SECTOR

.loader_cpm_find_entry:
        push    bc
        call    loader_cpm_entry_matches
        pop     bc
        jr      z,.loader_cpm_find_found
        ld      de,#LOADER_CPM_ENTRY_SIZE
        add     ix,de
        djnz    .loader_cpm_find_entry

        ld      hl,(loader_cpm_directory_lba)
        inc     hl
        ld      (loader_cpm_directory_lba),hl
        ld      hl,#loader_cpm_directory_left
        dec     (hl)
        jr      nz,.loader_cpm_find_sector
        ld      a,#LOADER_CPM_ERR_NOT_FOUND
        or      a
        ret

.loader_cpm_find_floppy_error:
        add     a,#LOADER_CPM_ERR_FLOPPY_BASE
        or      a
        ret

.loader_cpm_find_found:
        xor     a
        ret

        ;; ------------------------------------------------------------
        ;; loader_cpm_entry_matches
        ;; Compare one CP/M directory entry. Attribute bits in the high bit of
        ;; filename bytes are ignored. Boot files are limited to four extents,
        ;; so a non-zero high extent field cannot match.
        ;;
        ;; Arguments: IX = directory entry
        ;; Returns: Z for a match, NZ otherwise
        ;;
        ;; Clobbers: AF, B, DE, HL
loader_cpm_entry_matches:
        ld      a,LOADER_CPM_DIRENT_USER(ix)
        ld      hl,#loader_cpm_user
        cp      (hl)
        ret     nz
        push    ix
        pop     de
        inc     de
        ld      hl,(loader_cpm_name)
        ld      b,#11

.loader_cpm_match_name:
        ld      a,(de)
        and     #0x7f
        cp      (hl)
        ret     nz
        inc     de
        inc     hl
        djnz    .loader_cpm_match_name

        ld      a,LOADER_CPM_DIRENT_EXTENT_LOW(ix)
        and     #0x1f
        ld      hl,#loader_cpm_extent
        cp      (hl)
        ret     nz
        ld      a,LOADER_CPM_DIRENT_EXTENT_HIGH(ix)
        and     #0x3f
        ret

        ;; ------------------------------------------------------------
        ;; loader_cpm_validate_extent
        ;; Check the CP/M 128-byte record count against the sector count from
        ;; the boot manifest: 64 sectors/128 records per logical extent.
        ;;
        ;; Arguments: IX = matching entry
        ;; Returns: Z when the count is consistent, NZ otherwise
        ;;
        ;; Clobbers: AF
loader_cpm_validate_extent:
        ld      a,(loader_cpm_sectors_left)
        cp      #64
        ld      a,#128
        jr      nc,.loader_cpm_validate_count
        ld      a,(loader_cpm_sectors_left)
        add     a,a
.loader_cpm_validate_count:
        cp      LOADER_CPM_DIRENT_RECORD_COUNT(ix)
        ret

        ;; ------------------------------------------------------------
        ;; loader_cpm_load_extent
        ;; Follow up to eight 16-bit allocation block pointers in one extent.
        ;;
        ;; Arguments: IX = matching entry
        ;; Returns: A = LOADER_CPM_OK/Z or error/NZ
        ;;
        ;; Clobbers: AF, BC, DE, HL
loader_cpm_load_extent:
        push    ix
        pop     hl
        ld      de,#LOADER_CPM_DIRENT_BLOCKS
        add     hl,de
        ld      b,#8

.loader_cpm_extent_block:
        ld      a,(loader_cpm_sectors_left)
        or      a
        ret     z
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        inc     hl
        push    bc
        push    hl
        call    loader_cpm_load_block
        pop     hl
        pop     bc
        or      a
        ret     nz
        djnz    .loader_cpm_extent_block
        xor     a
        ret

        ;; ------------------------------------------------------------
        ;; loader_cpm_load_block
        ;; Validate and translate one 2 KiB allocation block, then read up to
        ;; eight physical sectors from it.
        ;;
        ;; Arguments: DE = CP/M allocation block number
        ;; Returns: A = LOADER_CPM_OK/Z or error/NZ
        ;;
        ;; Clobbers: AF, BC, DE, HL
loader_cpm_load_block:
        ld      a,d
        or      a
        jr      nz,.loader_cpm_block_high
        ld      a,e
        cp      #2                     ; blocks 0 and 1 hold the directory
        jp      c,loader_cpm_corrupt_error
        jr      .loader_cpm_block_valid

.loader_cpm_block_high:
        cp      #1
        jp      nz,loader_cpm_corrupt_error
        ld      a,e
        cp      #0x56                  ; block 342 is outside the disk
        jp      nc,loader_cpm_corrupt_error

.loader_cpm_block_valid:
        ld      h,d
        ld      l,e
        add     hl,hl
        add     hl,hl
        add     hl,hl                  ; eight sectors per allocation block
        ld      de,#LOADER_CPM_BOOT_SECTORS
        add     hl,de
        ld      (loader_cpm_block_lba),hl

        ld      a,(loader_cpm_sectors_left)
        cp      #(LOADER_CPM_BLOCK_SECTORS + 1)
        jr      c,.loader_cpm_block_count
        ld      a,#LOADER_CPM_BLOCK_SECTORS
.loader_cpm_block_count:
        ld      (loader_cpm_block_left),a

.loader_cpm_block_sector:
        ld      de,(loader_cpm_block_lba)
        ld      hl,(loader_cpm_destination)
        call    floppy_read_sector
        jr      nz,.loader_cpm_block_floppy_error
        ld      hl,(loader_cpm_block_lba)
        inc     hl
        ld      (loader_cpm_block_lba),hl
        ld      hl,(loader_cpm_destination)
        inc     h                       ; destination is page aligned
        ld      (loader_cpm_destination),hl
        ld      hl,#loader_cpm_sectors_left
        dec     (hl)
        ld      hl,#loader_cpm_block_left
        dec     (hl)
        jr      nz,.loader_cpm_block_sector
        xor     a
        ret

.loader_cpm_block_floppy_error:
        add     a,#LOADER_CPM_ERR_FLOPPY_BASE
        or      a
        ret

loader_cpm_argument_error:
        ld      a,#LOADER_CPM_ERR_ARGUMENT
        or      a
        ret

loader_cpm_corrupt_error:
        ld      a,#LOADER_CPM_ERR_CORRUPT
        or      a
        ret

        ;; Private one-operation state. The first-stage reader is intentionally
        ;; synchronous and non-reentrant.
loader_cpm_user:
        .db     0x00
loader_cpm_name:
        .dw     0x0000
loader_cpm_destination:
        .dw     0x0000
loader_cpm_sectors_left:
        .db     0x00
loader_cpm_extent:
        .db     0x00
loader_cpm_directory_lba:
        .dw     0x0000
loader_cpm_directory_left:
        .db     0x00
loader_cpm_block_lba:
        .dw     0x0000
loader_cpm_block_left:
        .db     0x00
