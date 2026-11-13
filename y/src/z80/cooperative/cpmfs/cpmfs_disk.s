        .ifndef YOS_CPC_AMSDOS
        .equ YOS_CPC_AMSDOS,0
        .endif
        ;; cpmfs_disk.s
        ;;
        ;; Raw CP/M directory, extent, allocation-block, file-size, and sector
        ;; mapping operations for libcpmfs. Each caller supplies an IY file
        ;; node whose inline 256-byte DMA block is used for directory scans.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-08-12   tstih

        .module cpmfs_disk
        .optsdcc -mz80 sdcccall(1)

        .include "cpmfs.inc"

        .globl  __cpmfs_scan_size_iy
        .globl  __cpmfs_map_sector_iy
        .globl  __cpmfs_delete_iy
        .globl  __cpmfs_create_empty_iy
        .globl  __cpmfs_update_directory_iy
        .globl  __cpmfs_node_dma_iy
        .globl  __cpmfs_read_sector
        .globl  __cpmfs_write_sector
        .globl  __cpmfs_invalidate_iy
        .globl  __cpmfs_entry_matches_iy
        .globl  __cpmfs_zero
        .globl  __cpmfs_copy

        .area   _CODE

        ;; Read directory LBA DE into IY's DMA block.
.cpmfs_read_directory_iy:
        push    de
        call    __cpmfs_node_dma_iy
        pop     de
        ld      a,FILE_DRIVE(iy)
        jp      __cpmfs_read_sector

        ;; Write IY's DMA block to directory LBA DE.
.cpmfs_write_directory_iy:
        push    de
        call    __cpmfs_node_dma_iy
        pop     de
        ld      a,FILE_DRIVE(iy)
        jp      __cpmfs_write_sector

        ;; Return HL = entry at CPMFS_TMP_TARGET_INDEX in the node DMA.
.cpmfs_target_entry_iy:
        call    __cpmfs_node_dma_iy
        ld      a,(CPMFS_TMP_TARGET_INDEX)
        ld      e,a
        ld      d,#0
        ld      b,#5
.cpmfs_target_entry_shift:
        sla     e
        rl      d
        djnz    .cpmfs_target_entry_shift
        add     hl,de                 ; index * 32
        ret

        ;; Calculate entry extent number. HL=entry, returns DE=extent.
.cpmfs_entry_extent:
        push    hl
        ld      de,#DIRENT_S2
        add     hl,de
        ld      a,(hl)
        and     #0x3f
        ld      e,a
        ld      d,#0
        ld      b,#5
.cpmfs_entry_extent_shift:
        sla     e
        rl      d
        djnz    .cpmfs_entry_extent_shift
        pop     hl
        push    hl
        ld      bc,#DIRENT_EX
        add     hl,bc
        ld      a,(hl)
        and     #0x1f
        or      e
        ld      e,a
        pop     hl
        ret

        ;; Compare DE with CPMFS_TMP_EXTENT. Z means equal.
.cpmfs_compare_target_extent:
        call    __cpmfs_extent_base
        ld      hl,(CPMFS_TMP_EXTENT)
        ld      a,l
        cp      e
        ret     nz
        ld      a,h
        cp      d
        ret

        ;; Find the target extent and leave its directory sector in DMA.
        ;; IY=node, CPMFS_TMP_EXTENT set. Returns A=0 or 1.
.cpmfs_find_extent_iy:
        ld      hl,(CPMFS_G_DIR)
        ld      (CPMFS_TMP_LBA),hl
.cpmfs_find_extent_sector:
        ld      de,(CPMFS_TMP_LBA)
        call    .cpmfs_read_directory_iy
        or      a
        jr      nz,.cpmfs_find_extent_fail
        call    __cpmfs_node_dma_iy
        xor     a
        ld      (CPMFS_TMP_ENTRY_INDEX),a
.cpmfs_find_extent_entry:
        call    __cpmfs_entry_matches_iy
        jr      nz,.cpmfs_find_extent_next_entry
        call    .cpmfs_entry_extent
        push    hl
        call    .cpmfs_compare_target_extent
        pop     hl
        jr      z,.cpmfs_find_extent_hit
.cpmfs_find_extent_next_entry:
        ld      de,#CPMFS_DIRENT_SIZE
        add     hl,de
        ld      a,(CPMFS_TMP_ENTRY_INDEX)
        inc     a
        ld      (CPMFS_TMP_ENTRY_INDEX),a
        cp      #CPMFS_ENTRIES_PER_SECTOR
        jr      c,.cpmfs_find_extent_entry
        ld      hl,(CPMFS_TMP_LBA)
        inc     hl
        ld      (CPMFS_TMP_LBA),hl
        ld      de,(CPMFS_G_END)
        or      a
        sbc     hl,de
        jr      c,.cpmfs_find_extent_sector
.cpmfs_find_extent_fail:
        ld      a,#1
        or      a
        ret
.cpmfs_find_extent_hit:
        ld      (CPMFS_TMP_ENTRY),hl
        ld      de,(CPMFS_TMP_LBA)
        ld      (CPMFS_TMP_TARGET_LBA),de
        ld      a,(CPMFS_TMP_ENTRY_INDEX)
        ld      (CPMFS_TMP_TARGET_INDEX),a
        xor     a
        ret

        ;; Create CPMFS_TMP_EXTENT in the first deleted directory slot.
.cpmfs_create_extent_iy:
        ld      hl,(CPMFS_G_DIR)
        ld      (CPMFS_TMP_LBA),hl
.cpmfs_create_extent_sector:
        ld      de,(CPMFS_TMP_LBA)
        call    .cpmfs_read_directory_iy
        or      a
        jr      nz,.cpmfs_create_extent_fail
        call    __cpmfs_node_dma_iy
        xor     a
        ld      (CPMFS_TMP_ENTRY_INDEX),a
.cpmfs_create_extent_entry:
        ld      a,(hl)
        cp      #DIRENT_DELETED
        jr      z,.cpmfs_create_extent_slot
        ld      de,#CPMFS_DIRENT_SIZE
        add     hl,de
        ld      a,(CPMFS_TMP_ENTRY_INDEX)
        inc     a
        ld      (CPMFS_TMP_ENTRY_INDEX),a
        cp      #CPMFS_ENTRIES_PER_SECTOR
        jr      c,.cpmfs_create_extent_entry
        ld      hl,(CPMFS_TMP_LBA)
        inc     hl
        ld      (CPMFS_TMP_LBA),hl
        ld      de,(CPMFS_G_END)
        or      a
        sbc     hl,de
        jr      c,.cpmfs_create_extent_sector
.cpmfs_create_extent_fail:
        ld      a,#1
        or      a
        ret
.cpmfs_create_extent_slot:
        ld      (CPMFS_TMP_ENTRY),hl
        push    hl
        ld      b,#CPMFS_DIRENT_SIZE
        call    __cpmfs_zero
        pop     hl
        ld      a,FILE_AREA(iy)
        ld      (hl),a
        inc     hl
        ex      de,hl
        push    iy
        pop     hl
        ld      bc,#FILE_NAME
        add     hl,bc
        ld      b,#11
        call    __cpmfs_copy
        ex      de,hl
        ld      hl,(CPMFS_TMP_ENTRY)
        ld      de,#DIRENT_EX
        add     hl,de
        ld      de,(CPMFS_TMP_EXTENT)
        ld      a,e
        and     #0x1f
        ld      (hl),a
        inc     hl
        xor     a
        ld      (hl),a                  ; byte count
        inc     hl
        ld      a,e
        srl     d
        rr      a
        srl     d
        rr      a
        srl     d
        rr      a
        srl     d
        rr      a
        srl     d
        rr      a                       ; extent >> 5
        and     #0x3f
        ld      (hl),a
        ld      de,(CPMFS_TMP_LBA)
        call    .cpmfs_write_directory_iy
        or      a
        jr      nz,.cpmfs_create_extent_fail
        ld      de,(CPMFS_TMP_LBA)
        ld      (CPMFS_TMP_TARGET_LBA),de
        ld      a,(CPMFS_TMP_ENTRY_INDEX)
        ld      (CPMFS_TMP_TARGET_INDEX),a
        call    .cpmfs_target_entry_iy
        ld      (CPMFS_TMP_ENTRY),hl
        xor     a
        ret

        ;; Mark one allocation block in the 342-bit temporary bitmap.
        ;; DE=block; invalid values are ignored. Preserves BC and HL.
.cpmfs_mark_block:
        push    bc
        push    hl
        call    __cpmfs_block_valid
        jr      c,.cpmfs_mark_block_done
.cpmfs_mark_block_valid:
        ld      a,e
        and     #7
        ld      b,a
        ld      c,#1
.cpmfs_mark_mask_loop:
        ld      a,b
        or      a
        jr      z,.cpmfs_mark_mask_ready
        sla     c
        dec     b
        jr      .cpmfs_mark_mask_loop
.cpmfs_mark_mask_ready:
        ld      h,d
        ld      l,e
        srl     h
        rr      l
        srl     h
        rr      l
        srl     h
        rr      l
        ld      de,#CPMFS_ALLOC_MAP
        add     hl,de
        ld      a,(hl)
        or      c
        ld      (hl),a
.cpmfs_mark_block_done:
        pop     hl
        pop     bc
        ret

        ;; Rebuild the allocation bitmap from every live CP/M file entry.
.cpmfs_build_alloc_map_iy:
        ld      hl,#CPMFS_ALLOC_MAP
        ld      de,#CPMFS_ALLOC_MAP+1
        ld      bc,#CPMFS_ALLOC_MAP_SIZE-1
        ld      (hl),#0
        ldir
        ld      a,(CPMFS_G_RESERVED)
        cp      #8
        ld      a,#3
        jr      nz,.cpmfs_bitmap_reserved
        ld      a,#0xff
.cpmfs_bitmap_reserved:
        ld      (CPMFS_ALLOC_MAP),a
        ld      hl,(CPMFS_G_DIR)
        ld      (CPMFS_TMP_LBA),hl
.cpmfs_alloc_map_sector:
        ld      de,(CPMFS_TMP_LBA)
        call    .cpmfs_read_directory_iy
        or      a
        ret     nz
        call    __cpmfs_node_dma_iy
        ld      b,#CPMFS_ENTRIES_PER_SECTOR
.cpmfs_alloc_map_entry:
        ld      a,(hl)
        cp      #CPMFS_AREAS
        jr      nc,.cpmfs_alloc_map_next_entry
        push    bc
        push    hl
        ld      de,#DIRENT_BLOCKS
        add     hl,de
        .if YOS_CPC_AMSDOS
        ld      b,#16
        .else
        ld      b,#8
        .endif
.cpmfs_alloc_map_pointer:
        ld      e,(hl)
        inc     hl
        .if YOS_CPC_AMSDOS
        ld      d,#0
        .else
        ld      d,(hl)
        inc     hl
        .endif
        ld      a,d
        or      e
        call    nz,.cpmfs_mark_block
        djnz    .cpmfs_alloc_map_pointer
        pop     hl
        pop     bc
.cpmfs_alloc_map_next_entry:
        ld      de,#CPMFS_DIRENT_SIZE
        add     hl,de
        djnz    .cpmfs_alloc_map_entry
        ld      hl,(CPMFS_TMP_LBA)
        inc     hl
        ld      (CPMFS_TMP_LBA),hl
        ld      de,(CPMFS_G_END)
        or      a
        sbc     hl,de
        jr      c,.cpmfs_alloc_map_sector
        xor     a
        ret

        ;; Test bitmap bit for DE block. Return Z when free, NZ when used.
.cpmfs_test_block:
        push    bc
        push    hl
        ld      a,e
        and     #7
        ld      b,a
        ld      c,#1
.cpmfs_test_mask_loop:
        ld      a,b
        or      a
        jr      z,.cpmfs_test_mask_ready
        sla     c
        dec     b
        jr      .cpmfs_test_mask_loop
.cpmfs_test_mask_ready:
        ld      h,d
        ld      l,e
        srl     h
        rr      l
        srl     h
        rr      l
        srl     h
        rr      l
        ld      de,#CPMFS_ALLOC_MAP
        add     hl,de
        ld      a,(hl)
        and     c
        pop     hl
        pop     bc
        ret

        ;; Allocate one free block and install it in the target dir entry.
        ;; Returns DE=block and A=0, or A=1.
.cpmfs_allocate_block_iy:
        call    .cpmfs_build_alloc_map_iy
        or      a
        jr      nz,.cpmfs_allocate_block_fail
        ld      a,(CPMFS_G_RESERVED)
        ld      e,a
        ld      d,#0
.cpmfs_allocate_block_scan:
        push    de
        call    .cpmfs_test_block
        pop     de
        jr      z,.cpmfs_allocate_block_found
        inc     de
        call    __cpmfs_block_valid
        jr      nc,.cpmfs_allocate_block_scan
.cpmfs_allocate_block_fail:
        ld      a,#1
        or      a
        ret
.cpmfs_allocate_block_found:
        ld      (CPMFS_TMP_BLOCK),de
        ld      de,(CPMFS_TMP_TARGET_LBA)
        call    .cpmfs_read_directory_iy
        or      a
        jr      nz,.cpmfs_allocate_block_fail
        call    .cpmfs_target_entry_iy
        ld      de,#DIRENT_BLOCKS
        add     hl,de
        ld      a,(CPMFS_TMP_BLOCK_SLOT)
        .if YOS_CPC_AMSDOS
        .else
        add     a,a
        .endif
        ld      e,a
        ld      d,#0
        add     hl,de
        ld      de,(CPMFS_TMP_BLOCK)
        ld      (hl),e
        .if YOS_CPC_AMSDOS
        .else
        inc     hl
        ld      (hl),d
        .endif
        ld      de,(CPMFS_TMP_TARGET_LBA)
        call    .cpmfs_write_directory_iy
        or      a
        jr      nz,.cpmfs_allocate_block_fail
        ld      de,(CPMFS_TMP_BLOCK)
        xor     a
        ret

        ;; ------------------------------------------------------------
        ;; __cpmfs_map_sector_iy
        ;; Translate a file-relative 256-byte sector to physical LBA, creating
        ;; an extent/allocation pointer when A is nonzero.
        ;;
        ;; Arguments: IY=node, HL=file sector, A=create
        ;; Returns: DE=LBA and A=0, or A=1
__cpmfs_map_sector_iy::
        call    __cpmfs_geometry_iy
        ld      (CPMFS_TMP_CREATE),a
        ld      (CPMFS_TMP_SECTOR),hl
        .if YOS_CPC_AMSDOS
        ld a,l
        and #3
        ld (CPMFS_TMP_IN_BLOCK),a
        srl h
        rr l
        srl h
        rr l
        ld a,l
        and #15
        ld (CPMFS_TMP_BLOCK_SLOT),a
        .rept 4
        srl h
        rr l
        .endm
        .else
        ld      a,(CPMFS_G_SHIFT)
        ld      b,a
        ld      a,l
        and     #7
        ld      c,a
        ld      a,b
        cp      #4
        jr      nz,.cpmfs_map_floppy_bits
        ld      a,l
        and     #15
        ld      c,a
.cpmfs_map_floppy_bits:
        ld      a,c
        ld      (CPMFS_TMP_IN_BLOCK),a
.cpmfs_map_block_shift:
        srl     h
        rr      l
        djnz    .cpmfs_map_block_shift
        ld      a,l
        and     #7
        ld      (CPMFS_TMP_BLOCK_SLOT),a
        srl     h
        rr      l
        srl     h
        rr      l
        srl     h
        rr      l
        ld      a,(CPMFS_G_EXM)
        or      a
        jr      z,.cpmfs_map_extent_ready
        add     hl,hl                  ; physical extent -> logical extent
        .endif
.cpmfs_map_extent_ready:
        ld      (CPMFS_TMP_EXTENT),hl
        call    __cpmfs_invalidate_iy
        or      a
        jr      nz,.cpmfs_map_fail
        call    .cpmfs_find_extent_iy
        or      a
        jr      z,.cpmfs_map_have_extent
        ld      a,(CPMFS_TMP_CREATE)
        or      a
        jr      z,.cpmfs_map_fail
        call    .cpmfs_create_extent_iy
        or      a
        jr      nz,.cpmfs_map_fail
.cpmfs_map_have_extent:
        ld      hl,(CPMFS_TMP_ENTRY)
        ld      de,#DIRENT_BLOCKS
        add     hl,de
        ld      a,(CPMFS_TMP_BLOCK_SLOT)
        .if YOS_CPC_AMSDOS
        .else
        add     a,a
        .endif
        ld      e,a
        ld      d,#0
        add     hl,de
        ld      e,(hl)
        .if YOS_CPC_AMSDOS
        ld      d,#0
        ld      a,e
        or      a
        .else
        inc     hl
        ld      d,(hl)
        ld      a,d
        or      e
        .endif
        jr      nz,.cpmfs_map_validate_block
        ld      a,(CPMFS_TMP_CREATE)
        or      a
        jr      z,.cpmfs_map_fail
        call    .cpmfs_allocate_block_iy
        or      a
        jr      nz,.cpmfs_map_fail
.cpmfs_map_validate_block:
        call    __cpmfs_block_valid
        jr      c,.cpmfs_map_fail
        ld      a,d
        or      a
        jr      nz,.cpmfs_map_block_valid
        ld      a,(CPMFS_G_RESERVED)
        cp      e
        jr      z,.cpmfs_map_block_valid
        jr      nc,.cpmfs_map_fail
.cpmfs_map_block_valid:
        ld      h,d
        ld      l,e
        ld      a,(CPMFS_G_SHIFT)
        ld      b,a
.cpmfs_map_lba_shift:
        add     hl,hl
        djnz    .cpmfs_map_lba_shift
        ld      de,(CPMFS_G_DIR)
        add     hl,de
        ld      a,(CPMFS_TMP_IN_BLOCK)
        ld      e,a
        ld      d,#0
        add     hl,de
        ex      de,hl
        xor     a
        ret
.cpmfs_map_fail:
        ld      a,#1
        or      a
        ret

        ;; Shift the 32-bit CPMFS_TMP_VALUE left seven bits.
.cpmfs_value_shift_left7:
        ld      b,#7
.cpmfs_value_shift_left_loop:
        ld      hl,#CPMFS_TMP_VALUE
        sla     (hl)
        inc     hl
        rl      (hl)
        inc     hl
        rl      (hl)
        inc     hl
        rl      (hl)
        djnz    .cpmfs_value_shift_left_loop
        ret

        ;; Shift the 32-bit CPMFS_TMP_VALUE right seven bits.
.cpmfs_value_shift_right7:
        ld      b,#7
.cpmfs_value_shift_right_loop:
        ld      hl,#CPMFS_TMP_VALUE + 3
        srl     (hl)
        dec     hl
        rr      (hl)
        dec     hl
        rr      (hl)
        dec     hl
        rr      (hl)
        djnz    .cpmfs_value_shift_right_loop
        ret

        ;; Compare TMP_VALUE candidate against TMP_SIZE; copy when greater.
.cpmfs_keep_larger_size:
        ld      hl,#CPMFS_TMP_VALUE + 3
        ld      de,#CPMFS_TMP_SIZE + 3
        ld      b,#4
.cpmfs_size_compare_loop:
        ld      a,(de)
        cp      (hl)
        jr      c,.cpmfs_size_copy
        ret     nz
        dec     hl
        dec     de
        djnz    .cpmfs_size_compare_loop
        ret
.cpmfs_size_copy:
        ld      hl,#CPMFS_TMP_VALUE
        ld      de,#CPMFS_TMP_SIZE
        ld      b,#4
        jp      __cpmfs_copy

        ;; ------------------------------------------------------------
        ;; __cpmfs_scan_size_iy
        ;; Find all matching extents and reconstruct exact CP/M 3 byte length.
        ;;
        ;; Returns: A=0 with FILE_SIZE_BYTES set, or A=1 when absent/I/O error
__cpmfs_scan_size_iy::
        call    __cpmfs_geometry_iy
        call    __cpmfs_invalidate_iy
        or      a
        jp      nz,.cpmfs_scan_size_fail
        ld      hl,#CPMFS_TMP_SIZE
        ld      b,#4
        call    __cpmfs_zero
        xor     a
        ld      (CPMFS_TMP_FOUND),a
        ld      hl,(CPMFS_G_DIR)
        ld      (CPMFS_TMP_LBA),hl
.cpmfs_scan_size_sector:
        ld      de,(CPMFS_TMP_LBA)
        call    .cpmfs_read_directory_iy
        or      a
        jp      nz,.cpmfs_scan_size_fail
        call    __cpmfs_node_dma_iy
        ld      b,#CPMFS_ENTRIES_PER_SECTOR
.cpmfs_scan_size_entry:
        push    bc
        call    __cpmfs_entry_matches_iy
        jr      nz,.cpmfs_scan_size_next
        ld      a,#1
        ld      (CPMFS_TMP_FOUND),a
        push    hl
        call    .cpmfs_entry_extent
        ld      a,e
        ld      (CPMFS_TMP_VALUE),a
        ld      a,d
        ld      (CPMFS_TMP_VALUE + 1),a
        xor     a
        ld      (CPMFS_TMP_VALUE + 2),a
        ld      (CPMFS_TMP_VALUE + 3),a
        call    .cpmfs_value_shift_left7
        pop     hl
        push    hl
        ld      de,#DIRENT_RC
        add     hl,de
        ld      a,(hl)
        ld      hl,#CPMFS_TMP_VALUE
        add     a,(hl)
        ld      (hl),a
        jr      nc,.cpmfs_scan_size_records_ready
        inc     hl
        inc     (hl)
        jr      nz,.cpmfs_scan_size_records_ready
        inc     hl
        inc     (hl)
.cpmfs_scan_size_records_ready:
        call    .cpmfs_value_shift_left7
        pop     hl
        push    hl
        ld      de,#6
        add     hl,de
        bit     7,(hl)
        pop     hl
        jr      z,.cpmfs_scan_size_keep
        push    hl
        ld      de,#DIRENT_RC
        add     hl,de
        ld      a,(hl)
        pop     hl
        or      a
        jr      z,.cpmfs_scan_size_keep
        push    hl
        ld      de,#DIRENT_BC
        add     hl,de
        ld      a,(hl)
        pop     hl
        or      a
        jr      z,.cpmfs_scan_size_keep
        cp      #128
        jr      nc,.cpmfs_scan_size_keep
        ld      c,a
        ld      de,#128
        ld      hl,(CPMFS_TMP_VALUE)
        or      a
        sbc     hl,de
        ld      (CPMFS_TMP_VALUE),hl
        ld      hl,#CPMFS_TMP_VALUE
        ld      a,(hl)
        add     a,c
        ld      (hl),a
        jr      nc,.cpmfs_scan_size_keep
        inc     hl
        inc     (hl)
.cpmfs_scan_size_keep:
        push    hl
        call    .cpmfs_keep_larger_size
        pop     hl
.cpmfs_scan_size_next:
        pop     bc
        ld      de,#CPMFS_DIRENT_SIZE
        add     hl,de
        djnz    .cpmfs_scan_size_entry
        ld      hl,(CPMFS_TMP_LBA)
        inc     hl
        ld      (CPMFS_TMP_LBA),hl
        ld      de,(CPMFS_G_END)
        or      a
        sbc     hl,de
        jp      c,.cpmfs_scan_size_sector
        ld      a,(CPMFS_TMP_FOUND)
        or      a
        jr      z,.cpmfs_scan_size_fail
        push    iy
        pop     de
        ld      hl,#FILE_SIZE_BYTES
        add     hl,de
        ex      de,hl
        ld      hl,#CPMFS_TMP_SIZE
        ld      b,#4
        call    __cpmfs_copy
        xor     a
        ret
.cpmfs_scan_size_fail:
        ld      a,#1
        or      a
        ret

        ;; ------------------------------------------------------------
        ;; __cpmfs_delete_iy
        ;; Mark all matching file extents deleted, releasing their blocks.
__cpmfs_delete_iy::
        call    __cpmfs_geometry_iy
        call    __cpmfs_invalidate_iy
        or      a
        jp      nz,.cpmfs_delete_fail
        xor     a
        ld      (CPMFS_TMP_FOUND),a
        ld      hl,(CPMFS_G_DIR)
        ld      (CPMFS_TMP_LBA),hl
.cpmfs_delete_sector:
        ld      de,(CPMFS_TMP_LBA)
        call    .cpmfs_read_directory_iy
        or      a
        jr      nz,.cpmfs_delete_fail
        xor     a
        ld      (CPMFS_TMP_DIRTY),a
        call    __cpmfs_node_dma_iy
        ld      b,#CPMFS_ENTRIES_PER_SECTOR
.cpmfs_delete_entry:
        push    bc
        call    __cpmfs_entry_matches_iy
        jr      nz,.cpmfs_delete_next
        ld      (hl),#DIRENT_DELETED
        ld      a,#1
        ld      (CPMFS_TMP_DIRTY),a
        ld      (CPMFS_TMP_FOUND),a
.cpmfs_delete_next:
        pop     bc
        ld      de,#CPMFS_DIRENT_SIZE
        add     hl,de
        djnz    .cpmfs_delete_entry
        ld      a,(CPMFS_TMP_DIRTY)
        or      a
        jr      z,.cpmfs_delete_sector_done
        ld      de,(CPMFS_TMP_LBA)
        call    .cpmfs_write_directory_iy
        or      a
        jr      nz,.cpmfs_delete_fail
.cpmfs_delete_sector_done:
        ld      hl,(CPMFS_TMP_LBA)
        inc     hl
        ld      (CPMFS_TMP_LBA),hl
        ld      de,(CPMFS_G_END)
        or      a
        sbc     hl,de
        jr      c,.cpmfs_delete_sector
        ld      a,(CPMFS_TMP_FOUND)
        or      a
        jr      z,.cpmfs_delete_fail
        xor     a
        ret
.cpmfs_delete_fail:
        ld      a,#1
        or      a
        ret

        ;; Create an empty extent zero for IY's name.
__cpmfs_create_empty_iy::
        call    __cpmfs_geometry_iy
        ld      hl,#0
        ld      (CPMFS_TMP_EXTENT),hl
        jp      .cpmfs_create_extent_iy

        ;; ------------------------------------------------------------
        ;; __cpmfs_update_directory_iy
        ;; Write record counts and CP/M 3 last-byte counts for all extents.
__cpmfs_update_directory_iy::
        call    __cpmfs_geometry_iy
        call    __cpmfs_invalidate_iy
        or      a
        jp      nz,.cpmfs_update_fail
        ld      a,FILE_SIZE_BYTES(iy)
        and     #0x7f
        ld      (CPMFS_TMP_BLOCK_SLOT),a ; final byte count
        ld      a,FILE_SIZE_BYTES(iy)
        ld      (CPMFS_TMP_VALUE),a
        ld      a,FILE_SIZE_BYTES + 1(iy)
        ld      (CPMFS_TMP_VALUE + 1),a
        ld      a,FILE_SIZE_BYTES + 2(iy)
        ld      (CPMFS_TMP_VALUE + 2),a
        ld      a,FILE_SIZE_BYTES + 3(iy)
        ld      (CPMFS_TMP_VALUE + 3),a
        ld      hl,#CPMFS_TMP_VALUE
        ld      a,(hl)
        add     a,#127
        ld      (hl),a
        jr      nc,.cpmfs_update_round_ready
        inc     hl
        inc     (hl)
        jr      nz,.cpmfs_update_round_ready
        inc     hl
        inc     (hl)
        jr      nz,.cpmfs_update_round_ready
        inc     hl
        inc     (hl)
.cpmfs_update_round_ready:
        call    .cpmfs_value_shift_right7 ; total 128-byte records
        ld      hl,(CPMFS_G_DIR)
        ld      (CPMFS_TMP_LBA),hl
.cpmfs_update_sector:
        ld      de,(CPMFS_TMP_LBA)
        call    .cpmfs_read_directory_iy
        or      a
        jp      nz,.cpmfs_update_fail
        xor     a
        ld      (CPMFS_TMP_DIRTY),a
        call    __cpmfs_node_dma_iy
        ld      b,#CPMFS_ENTRIES_PER_SECTOR
.cpmfs_update_entry:
        push    bc
        call    __cpmfs_entry_matches_iy
        jp      nz,.cpmfs_update_next
        push    hl
        call    .cpmfs_entry_extent
        call    __cpmfs_extent_base
        ld      a,e
        ld      (CPMFS_TMP_VALUE2),a
        ld      a,d
        ld      (CPMFS_TMP_VALUE2 + 1),a
        xor     a
        ld      (CPMFS_TMP_VALUE2 + 2),a
        ld      (CPMFS_TMP_VALUE2 + 3),a
        ld      b,#7
.cpmfs_update_base_shift:
        ld      hl,#CPMFS_TMP_VALUE2
        sla     (hl)
        inc     hl
        rl      (hl)
        inc     hl
        rl      (hl)
        inc     hl
        rl      (hl)
        djnz    .cpmfs_update_base_shift
        pop     hl
        push    hl
        ;; remaining records = total - extent*128
        ld      hl,#CPMFS_TMP_VALUE2
        ld      b,(hl)
        ld      a,(CPMFS_TMP_VALUE)
        sub     b
        ld      e,a
        inc     hl
        ld      b,(hl)
        ld      a,(CPMFS_TMP_VALUE + 1)
        sbc     a,b
        ld      d,a
        inc     hl
        ld      b,(hl)
        ld      a,(CPMFS_TMP_VALUE + 2)
        sbc     a,b
        ld      c,a
        inc     hl
        ld      b,(hl)
        ld      a,(CPMFS_TMP_VALUE + 3)
        sbc     a,b
        jr      c,.cpmfs_update_empty_count
        or      c
        jr      z,.cpmfs_update_count_ready
        ld      de,#0x100
        jr      .cpmfs_update_count_ready
.cpmfs_update_empty_count:
        ld      de,#0
.cpmfs_update_count_ready:
        pop     hl
        push    hl
        call    __cpmfs_record_count
        ;; Only the final physical extent carries the exact byte count.
        push    hl
        ld      bc,#13
        add     hl,bc
        ld      (hl),#0
        pop     hl
        push    hl
        ld      bc,#6
        add     hl,bc
        res     7,(hl)
        pop     hl
        ld      a,d
        or      a
        jr      nz,.cpmfs_update_mark
        ld      a,(CPMFS_G_EXM)
        or      a
        jr      nz,.cpmfs_update_final_count
        ld      a,e
        cp      #128
        jr      nc,.cpmfs_update_mark
.cpmfs_update_final_count:
        ld      a,e
        or      a
        jr      z,.cpmfs_update_mark
        push    hl
        ld      bc,#13
        add     hl,bc
        ld      a,(CPMFS_TMP_BLOCK_SLOT)
        ld      (hl),a
        pop     hl
        ld      bc,#6
        add     hl,bc
        set     7,(hl)
.cpmfs_update_mark:
        ld      a,#1
        ld      (CPMFS_TMP_DIRTY),a
        pop     hl
.cpmfs_update_next:
        pop     bc
        ld      de,#CPMFS_DIRENT_SIZE
        add     hl,de
        dec     b
        jp      nz,.cpmfs_update_entry
        ld      a,(CPMFS_TMP_DIRTY)
        or      a
        jr      z,.cpmfs_update_sector_done
        ld      de,(CPMFS_TMP_LBA)
        call    .cpmfs_write_directory_iy
        or      a
        jr      nz,.cpmfs_update_fail
.cpmfs_update_sector_done:
        ld      hl,(CPMFS_TMP_LBA)
        inc     hl
        ld      (CPMFS_TMP_LBA),hl
        ld      de,(CPMFS_G_END)
        or      a
        sbc     hl,de
        jp      c,.cpmfs_update_sector
        xor     a
        ret
.cpmfs_update_fail:
        ld      a,#1
        or      a
        ret
