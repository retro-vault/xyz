        ;; cpmfs_namespace.s
        ;;
        ;; Filesystem namespace operations that rewrite raw CP/M directory
        ;; entries. A rename changes every extent and updates matching live
        ;; descriptors so their later metadata flushes use the new pathname.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-08-12   tstih

        .module cpmfs_namespace
        .optsdcc -mz80 sdcccall(1)

        .include "cpmfs.inc"

        .globl  _rename
        .globl  _cpmfs_init
        .globl  _cpmfs_parse_path
        .globl  __cpmfs_set_error
        .globl  __cpmfs_copy
        .globl  __cpmfs_entry_matches_iy
        .globl  __cpmfs_node_dma_iy
        .globl  __cpmfs_read_sector
        .globl  __cpmfs_write_sector
        .globl  __cpmfs_scan_size_iy
        .globl  _smalloc
        .globl  _sfree
        .globl  _list_front
        .globl  _list_next

        .area   _CODE

        ;; Populate the path-dependent portion of IY's file node from HL.
.cpmfs_node_from_path_iy:
        ld      a,(hl)
        ld      FILE_DRIVE(iy),a
        inc     hl
        ld      a,(hl)
        ld      FILE_AREA(iy),a
        ld      de,#(PATH_CANONICAL - PATH_AREA)
        add     hl,de
        push    hl
        push    iy
        pop     de
        ld      bc,#FILE_NAME
        ex      de,hl
        add     hl,bc
        ex      de,hl
        pop     hl
        ld      b,#11
        jp      __cpmfs_copy

        ;; Return Z when PATH1 and PATH2 identify the same device/area/8.3 name.
.cpmfs_paths_equal:
        ld      a,(CPMFS_TMP_PATH1 + PATH_DRIVE)
        ld      b,a
        ld      a,(CPMFS_TMP_PATH2 + PATH_DRIVE)
        cp      b
        ret     nz
        ld      a,(CPMFS_TMP_PATH1 + PATH_AREA)
        ld      b,a
        ld      a,(CPMFS_TMP_PATH2 + PATH_AREA)
        cp      b
        ret     nz
        ld      hl,#CPMFS_TMP_PATH1 + PATH_CANONICAL
        ld      de,#CPMFS_TMP_PATH2 + PATH_CANONICAL
        ld      b,#11
.cpmfs_paths_equal_loop:
        ld      a,(de)
        cp      (hl)
        ret     nz
        inc     hl
        inc     de
        djnz    .cpmfs_paths_equal_loop
        xor     a
        ret

        ;; Update all linked descriptors that still carry the old pathname.
.cpmfs_rename_open_nodes:
        ld      hl,#CPMFS_STATE_HEAD
        call    _list_front
        ex      de,hl
.cpmfs_rename_open_loop:
        ld      a,h
        or      l
        ret     z
        push    hl
        pop     ix
        ld      a,FILE_DRIVE(ix)
        ld      b,a
        ld      a,(CPMFS_TMP_PATH1 + PATH_DRIVE)
        cp      b
        jr      nz,.cpmfs_rename_open_next
        ld      a,FILE_AREA(ix)
        ld      b,a
        ld      a,(CPMFS_TMP_PATH1 + PATH_AREA)
        cp      b
        jr      nz,.cpmfs_rename_open_next
        push    ix
        pop     hl
        ld      de,#FILE_NAME
        add     hl,de
        ld      de,#CPMFS_TMP_PATH1 + PATH_CANONICAL
        ld      b,#11
.cpmfs_rename_open_compare:
        ld      a,(de)
        cp      (hl)
        jr      nz,.cpmfs_rename_open_next
        inc     hl
        inc     de
        djnz    .cpmfs_rename_open_compare
        ld      a,(CPMFS_TMP_PATH2 + PATH_AREA)
        ld      FILE_AREA(ix),a
        ld      hl,#CPMFS_TMP_PATH2 + PATH_CANONICAL
        push    ix
        pop     de
        ld      bc,#FILE_NAME
        ex      de,hl
        add     hl,bc
        ex      de,hl
        ld      b,#11
        call    __cpmfs_copy
.cpmfs_rename_open_next:
        push    ix
        pop     hl
        call    _list_next
        ex      de,hl
        jr      .cpmfs_rename_open_loop

        ;; Rename every matching extent in IY's private directory DMA.
.cpmfs_rename_entries_iy:
        call    __cpmfs_geometry_iy
        xor     a
        ld      (CPMFS_TMP_FOUND),a
        ld      hl,(CPMFS_G_DIR)
        ld      (CPMFS_TMP_LBA),hl
.cpmfs_rename_sector:
        ld      de,(CPMFS_TMP_LBA)
        push    de
        call    __cpmfs_node_dma_iy
        pop     de
        ld      a,FILE_DRIVE(iy)
        call    __cpmfs_read_sector
        or      a
        jp      nz,.cpmfs_rename_entries_fail
        xor     a
        ld      (CPMFS_TMP_DIRTY),a
        call    __cpmfs_node_dma_iy
        ld      b,#CPMFS_ENTRIES_PER_SECTOR
.cpmfs_rename_entry:
        push    bc
        call    __cpmfs_entry_matches_iy
        jr      nz,.cpmfs_rename_next_entry
        push    hl
        ld      a,(CPMFS_TMP_PATH2 + PATH_AREA)
        ld      (hl),a
        inc     hl
        ex      de,hl                   ; DE = directory name
        ld      hl,#CPMFS_TMP_PATH2 + PATH_CANONICAL
        ld      b,#11
.cpmfs_rename_name_loop:
        ld      a,(de)
        and     #0x80                   ; preserve CP/M attribute flags
        xor     (hl)                    ; canonical bytes have bit 7 clear
        ld      (de),a
        inc     hl
        inc     de
        djnz    .cpmfs_rename_name_loop
        pop     hl
        ld      a,#1
        ld      (CPMFS_TMP_DIRTY),a
        ld      (CPMFS_TMP_FOUND),a
.cpmfs_rename_next_entry:
        pop     bc
        ld      de,#CPMFS_DIRENT_SIZE
        add     hl,de
        djnz    .cpmfs_rename_entry
        ld      a,(CPMFS_TMP_DIRTY)
        or      a
        jr      z,.cpmfs_rename_sector_done
        ld      de,(CPMFS_TMP_LBA)
        push    de
        call    __cpmfs_node_dma_iy
        pop     de
        ld      a,FILE_DRIVE(iy)
        call    __cpmfs_write_sector
        or      a
        jr      nz,.cpmfs_rename_entries_fail
.cpmfs_rename_sector_done:
        ld      hl,(CPMFS_TMP_LBA)
        inc     hl
        ld      (CPMFS_TMP_LBA),hl
        ld      de,(CPMFS_G_END)
        or      a
        sbc     hl,de
        jp      c,.cpmfs_rename_sector
        ld      a,(CPMFS_TMP_FOUND)
        or      a
        jr      z,.cpmfs_rename_entries_fail
        xor     a
        ret
.cpmfs_rename_entries_fail:
        ld      a,#1
        or      a
        ret

        ;; ------------------------------------------------------------
        ;; rename
        ;; CP/M-style rename: destination must not already exist, and movement
        ;; is permitted between user areas but not between physical devices.
_rename::
        push    ix
        push    iy
        ld      (CPMFS_TMP_FLAGS),de     ; preserve newpath across init/parser
        push    hl
        call    _cpmfs_init
        ld      a,d
        and     e
        inc     a
        pop     hl
        jp      z,.cpmfs_rename_memory_fail_no_node
        ld      de,#CPMFS_TMP_PATH1
        call    _cpmfs_parse_path
        ld      a,d
        and     e
        inc     a
        jp      z,.cpmfs_rename_path_fail_no_node
        ld      hl,(CPMFS_TMP_FLAGS)
        ld      de,#CPMFS_TMP_PATH2
        call    _cpmfs_parse_path
        ld      a,d
        and     e
        inc     a
        jp      z,.cpmfs_rename_path_fail_no_node
        ld      a,(CPMFS_TMP_PATH1 + PATH_DRIVE)
        ld      b,a
        ld      a,(CPMFS_TMP_PATH2 + PATH_DRIVE)
        cp      b
        jp      nz,.cpmfs_rename_drive_fail_no_node
        ld      a,b
        cp      #5
        jp      nc,.cpmfs_rename_device_fail_no_node

        ld      hl,#FILE_NODE_SIZE
        call    _smalloc
        ld      a,d
        or      e
        jp      z,.cpmfs_rename_memory_fail_no_node
        ld      (CPMFS_TMP_NODE),de
        push    de
        pop     iy
        push    iy
        pop     hl
        ld      bc,#FILE_NODE_SIZE
.cpmfs_rename_zero_loop:
        xor     a
        ld      (hl),a
        inc     hl
        dec     bc
        ld      a,b
        or      c
        jr      nz,.cpmfs_rename_zero_loop
        ld      hl,#CPMFS_TMP_PATH1
        call    .cpmfs_node_from_path_iy
        call    __cpmfs_scan_size_iy
        or      a
        jp      nz,.cpmfs_rename_not_found

        call    .cpmfs_paths_equal
        jr      z,.cpmfs_rename_success

        ld      hl,#CPMFS_TMP_PATH2
        call    .cpmfs_node_from_path_iy
        call    __cpmfs_scan_size_iy
        or      a
        jp      z,.cpmfs_rename_exists

        ld      hl,#CPMFS_TMP_PATH1
        call    .cpmfs_node_from_path_iy
        call    .cpmfs_rename_entries_iy
        or      a
        jp      nz,.cpmfs_rename_io_fail
        call    .cpmfs_rename_open_nodes
.cpmfs_rename_success:
        ld      hl,(CPMFS_TMP_NODE)
        call    _sfree
        xor     a
        call    __cpmfs_set_error
        ld      de,#0
        pop     iy
        pop     ix
        ret
.cpmfs_rename_not_found:
        ld      a,#CPMFS_ERR_NOT_FOUND
        jr      .cpmfs_rename_release_fail
.cpmfs_rename_exists:
        ld      a,#CPMFS_ERR_EXISTS
        jr      .cpmfs_rename_release_fail
.cpmfs_rename_io_fail:
        ld      a,#CPMFS_ERR_IO
.cpmfs_rename_release_fail:
        push    af
        ld      hl,(CPMFS_TMP_NODE)
        call    _sfree
        pop     af
        call    __cpmfs_set_error
        ld      de,#0xffff
        pop     iy
        pop     ix
        ret
.cpmfs_rename_path_fail_no_node:
        ld      a,#CPMFS_ERR_PATH
        jr      .cpmfs_rename_fail_no_node
.cpmfs_rename_drive_fail_no_node:
        ld      a,#CPMFS_ERR_ARGUMENT
        jr      .cpmfs_rename_fail_no_node
.cpmfs_rename_device_fail_no_node:
        ld      a,#CPMFS_ERR_DEVICE
        jr      .cpmfs_rename_fail_no_node
.cpmfs_rename_memory_fail_no_node:
        ld      a,#CPMFS_ERR_MEMORY
.cpmfs_rename_fail_no_node:
        call    __cpmfs_set_error
        ld      de,#0xffff
        pop     iy
        pop     ix
        ret
