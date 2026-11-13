        ;; cpmfs_file.s
        ;;
        ;; Linked-list file descriptors and POSIX-style open/close/read/write,
        ;; seek, control, and metadata entry points for libcpmfs. Every node is
        ;; one shared-heap allocation containing its own 256-byte DMA cache.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-08-12   tstih

        .module cpmfs_file
        .optsdcc -mz80 sdcccall(1)

        .include "cpmfs.inc"

        .globl  _open
        .globl  _creat
        .globl  _close
        .globl  _read
        .globl  _write
        .globl  _fcntl
        .globl  _fstat
        .globl  _lseek
        .globl  _chdir
        .globl  _stat
        .globl  _cpmfs_init
        .globl  _cpmfs_parse_path
        .globl  __cpmfs_parse_directory
        .globl  __cpmfs_set_error
        .globl  __cpmfs_zero
        .globl  __cpmfs_copy
        .globl  __cpmfs_find_fd
        .globl  __cpmfs_node_dma_iy
        .globl  __cpmfs_read_sector
        .globl  __cpmfs_flush_iy
        .globl  __cpmfs_invalidate_iy
        .globl  __cpmfs_scan_size_iy
        .globl  __cpmfs_map_sector_iy
        .globl  __cpmfs_delete_iy
        .globl  __cpmfs_create_empty_iy
        .globl  __cpmfs_update_directory_iy
        .globl  _smalloc
        .globl  _sfree
        .globl  _list_push_front
        .globl  _list_remove

        .equ    S_IFREG_PERMISSIONS,       0x81b6

        .area   _CODE

        ;; Store a zero 32-bit value at FILE_POSITION in IY.
.cpmfs_zero_position_iy:
        xor     a
        ld      FILE_POSITION(iy),a
        ld      FILE_POSITION + 1(iy),a
        ld      FILE_POSITION + 2(iy),a
        ld      FILE_POSITION + 3(iy),a
        ret

        ;; Copy FILE_SIZE_BYTES to FILE_POSITION in IY.
.cpmfs_position_to_end_iy:
        ld      a,FILE_SIZE_BYTES(iy)
        ld      FILE_POSITION(iy),a
        ld      a,FILE_SIZE_BYTES + 1(iy)
        ld      FILE_POSITION + 1(iy),a
        ld      a,FILE_SIZE_BYTES + 2(iy)
        ld      FILE_POSITION + 2(iy),a
        ld      a,FILE_SIZE_BYTES + 3(iy)
        ld      FILE_POSITION + 3(iy),a
        ret

        ;; Compare position to size. Carry means position < size, Z equal.
.cpmfs_compare_position_size_iy:
        ld      a,FILE_POSITION + 3(iy)
        cp      FILE_SIZE_BYTES + 3(iy)
        ret     nz
        ld      a,FILE_POSITION + 2(iy)
        cp      FILE_SIZE_BYTES + 2(iy)
        ret     nz
        ld      a,FILE_POSITION + 1(iy)
        cp      FILE_SIZE_BYTES + 1(iy)
        ret     nz
        ld      a,FILE_POSITION(iy)
        cp      FILE_SIZE_BYTES(iy)
        ret

        ;; Add 16-bit TMP_CHUNK to the 32-bit file position.
.cpmfs_advance_position_iy:
        ld      hl,(CPMFS_TMP_CHUNK)
        ld      a,FILE_POSITION(iy)
        add     a,l
        ld      FILE_POSITION(iy),a
        ld      a,FILE_POSITION + 1(iy)
        adc     a,h
        ld      FILE_POSITION + 1(iy),a
        ld      a,FILE_POSITION + 2(iy)
        adc     a,#0
        ld      FILE_POSITION + 2(iy),a
        ld      a,FILE_POSITION + 3(iy)
        adc     a,#0
        ld      FILE_POSITION + 3(iy),a
        ret

        ;; size = max(size, position).
.cpmfs_grow_size_iy:
        call    .cpmfs_compare_position_size_iy
        ret     c
        ret     z
        ld      a,FILE_POSITION(iy)
        ld      FILE_SIZE_BYTES(iy),a
        ld      a,FILE_POSITION + 1(iy)
        ld      FILE_SIZE_BYTES + 1(iy),a
        ld      a,FILE_POSITION + 2(iy)
        ld      FILE_SIZE_BYTES + 2(iy),a
        ld      a,FILE_POSITION + 3(iy)
        ld      FILE_SIZE_BYTES + 3(iy),a
        ret

        ;; Return HL = low 16 bits of current file sector (position >> 8).
.cpmfs_position_sector_iy:
        ld      l,FILE_POSITION + 1(iy)
        ld      h,FILE_POSITION + 2(iy)
        ret

        ;; Return Z when current cached file sector equals HL, NZ when the
        ;; caller must load a different sector.
.cpmfs_cache_sector_matches_iy:
        ld      a,FILE_CACHE_VALID(iy)
        or      a
        jr      z,.cpmfs_cache_sector_miss
        ld      a,FILE_CACHE_SECTOR(iy)
        cp      l
        jr      nz,.cpmfs_cache_sector_miss
        ld      a,FILE_CACHE_SECTOR + 1(iy)
        cp      h
        jr      nz,.cpmfs_cache_sector_miss
        xor     a
        ret
.cpmfs_cache_sector_miss:
        ld      a,#1
        or      a
        ret

        ;; Install cache keys for file sector HL and physical LBA DE.
.cpmfs_install_cache_key_iy:
        ld      FILE_CACHE_SECTOR(iy),l
        ld      FILE_CACHE_SECTOR + 1(iy),h
        ld      FILE_CACHE_LBA(iy),e
        ld      FILE_CACHE_LBA + 1(iy),d
        ld      a,#1
        ld      FILE_CACHE_VALID(iy),a
        xor     a
        ld      FILE_CACHE_DIRTY(iy),a
        ret

        ;; ------------------------------------------------------------
        ;; open
        ;; Allocate and link an unbounded shared-memory file descriptor node.
        ;;
        ;; Signature: int open(const char *pathname, int flags)
        ;;
        ;; Arguments: HL=pathname, DE=flags
        ;; Returns: DE=fd or -1
_open::
        push    ix
        push    iy
        ld      (CPMFS_TMP_POINTER),hl
        ld      (CPMFS_TMP_FLAGS),de
        call    _cpmfs_init
        ld      a,d
        and     e
        inc     a
        jp      z,.cpmfs_open_memory_fail
        ld      hl,#FILE_NODE_SIZE
        call    _smalloc
        ld      a,d
        or      e
        jp      z,.cpmfs_open_memory_fail
        push    de
        pop     iy
        push    iy
        pop     hl
        ld      bc,#FILE_NODE_SIZE
.cpmfs_open_zero_loop:
        xor     a
        ld      (hl),a
        inc     hl
        dec     bc
        ld      a,b
        or      c
        jr      nz,.cpmfs_open_zero_loop

        ld      hl,(CPMFS_TMP_POINTER)
        ld      de,#CPMFS_TMP_PATH1
        call    _cpmfs_parse_path
        ld      a,d
        and     e
        inc     a
        jp      z,.cpmfs_open_path_fail
        ld      a,(CPMFS_TMP_PATH1 + PATH_DRIVE)
        ld      FILE_DRIVE(iy),a
        cp      #5
        jp      nc,.cpmfs_open_device_fail
        ld      a,(CPMFS_TMP_PATH1 + PATH_AREA)
        ld      FILE_AREA(iy),a
        ld      hl,#CPMFS_TMP_PATH1 + PATH_CANONICAL
        push    iy
        pop     de
        ld      bc,#FILE_NAME
        ex      de,hl
        add     hl,bc
        ex      de,hl
        ld      b,#11
        call    __cpmfs_copy
        ld      hl,(CPMFS_TMP_FLAGS)
        ld      a,l
        and     #O_ACCMODE_VALUE
        cp      #3
        jp      z,.cpmfs_open_argument_fail
        ld      a,h
        and     #(O_TRUNC_VALUE >> 8)
        jr      z,.cpmfs_open_flags_valid
        ld      a,l
        and     #O_ACCMODE_VALUE
        jp      z,.cpmfs_open_access_fail
.cpmfs_open_flags_valid:
        ld      FILE_FLAGS(iy),l
        ld      FILE_FLAGS + 1(iy),h
        call    __cpmfs_scan_size_iy
        or      a
        jr      nz,.cpmfs_open_missing
        ld      a,FILE_FLAGS + 1(iy)
        and     #(O_TRUNC_VALUE >> 8)
        jr      z,.cpmfs_open_ready
        call    __cpmfs_delete_iy
        or      a
        jp      nz,.cpmfs_open_io_fail
        call    __cpmfs_create_empty_iy
        or      a
        jp      nz,.cpmfs_open_space_fail
        ld      hl,#0
        ld      FILE_SIZE_BYTES(iy),l
        ld      FILE_SIZE_BYTES + 1(iy),h
        ld      FILE_SIZE_BYTES + 2(iy),l
        ld      FILE_SIZE_BYTES + 3(iy),h
        jr      .cpmfs_open_ready
.cpmfs_open_missing:
        ld      a,FILE_FLAGS + 1(iy)
        and     #(O_CREAT_VALUE >> 8)
        jp      z,.cpmfs_open_not_found
        call    __cpmfs_create_empty_iy
        or      a
        jp      nz,.cpmfs_open_space_fail
        xor     a
        ld      FILE_SIZE_BYTES(iy),a
        ld      FILE_SIZE_BYTES + 1(iy),a
        ld      FILE_SIZE_BYTES + 2(iy),a
        ld      FILE_SIZE_BYTES + 3(iy),a
.cpmfs_open_ready:
        call    .cpmfs_zero_position_iy
        ld      a,FILE_FLAGS + 1(iy)
        and     #(O_APPEND_VALUE >> 8)
        call    nz,.cpmfs_position_to_end_iy
        ld      hl,(CPMFS_STATE_NEXT_FD)
        ld      FILE_FD(iy),l
        ld      FILE_FD + 1(iy),h
        inc     hl
        ld      (CPMFS_STATE_NEXT_FD),hl
        ld      hl,#CPMFS_STATE_HEAD
        push    iy
        pop     de
        call    _list_push_front
        xor     a
        call    __cpmfs_set_error
        ld      e,FILE_FD(iy)
        ld      d,FILE_FD + 1(iy)
        pop     iy
        pop     ix
        ret

.cpmfs_open_path_fail:
        ld      a,#CPMFS_ERR_PATH
        jr      .cpmfs_open_release_error
.cpmfs_open_argument_fail:
        ld      a,#CPMFS_ERR_ARGUMENT
        jr      .cpmfs_open_release_error
.cpmfs_open_access_fail:
        ld      a,#CPMFS_ERR_ACCESS
        jr      .cpmfs_open_release_error
.cpmfs_open_device_fail:
        ld      a,#CPMFS_ERR_DEVICE
        jr      .cpmfs_open_release_error
.cpmfs_open_not_found:
        ld      a,#CPMFS_ERR_NOT_FOUND
        jr      .cpmfs_open_release_error
.cpmfs_open_space_fail:
        ld      a,#CPMFS_ERR_NOSPC
        jr      .cpmfs_open_release_error
.cpmfs_open_io_fail:
        ld      a,#CPMFS_ERR_IO
.cpmfs_open_release_error:
        push    af
        push    iy
        pop     hl
        call    _sfree
        pop     af
        call    __cpmfs_set_error
        ld      de,#0xffff
        pop     iy
        pop     ix
        ret
.cpmfs_open_memory_fail:
        ld      a,#CPMFS_ERR_MEMORY
        call    __cpmfs_set_error
        ld      de,#0xffff
        pop     iy
        pop     ix
        ret

        ;; creat(path, mode) is open(path, O_WRONLY|O_CREAT|O_TRUNC).
_creat::
        ld      de,#(O_WRONLY_VALUE | O_CREAT_VALUE | O_TRUNC_VALUE)
        jp      _open

        ;; Remove IY node from liblist's intrusive open-file list.
.cpmfs_unlink_node_iy:
        ld      hl,#CPMFS_STATE_HEAD
        push    iy
        pop     de
        jp      _list_remove

        ;; ------------------------------------------------------------
        ;; close
        ;; Flush metadata/cache, unlink the node, and release its allocation.
_close::
        push    ix
        push    iy
        call    __cpmfs_find_fd
        ld      a,h
        or      l
        jr      z,.cpmfs_close_bad_fd
        push    hl
        pop     iy
        xor     a
        ld      (CPMFS_TMP_FOUND),a
        call    __cpmfs_flush_iy
        jr      nz,.cpmfs_close_mark_error
        ld      a,FILE_FLAGS(iy)
        and     #O_ACCMODE_VALUE
        jr      z,.cpmfs_close_release
        call    __cpmfs_update_directory_iy
        jr      z,.cpmfs_close_release
.cpmfs_close_mark_error:
        ld      a,#1
        ld      (CPMFS_TMP_FOUND),a
.cpmfs_close_release:
        call    .cpmfs_unlink_node_iy
        push    iy
        pop     hl
        call    _sfree
        ld      a,(CPMFS_TMP_FOUND)
        or      a
        jr      nz,.cpmfs_close_io_fail
        xor     a
        call    __cpmfs_set_error
        ld      de,#0
        pop     iy
        pop     ix
        ret
.cpmfs_close_bad_fd:
        ld      a,#CPMFS_ERR_BAD_FD
        jr      .cpmfs_close_fail
.cpmfs_close_io_fail:
        ld      a,#CPMFS_ERR_IO
.cpmfs_close_fail:
        call    __cpmfs_set_error
        ld      de,#0xffff
        pop     iy
        pop     ix
        ret

        ;; Compute min(remaining, 256-position_low) into TMP_CHUNK.
.cpmfs_prepare_chunk_iy:
        ld      a,FILE_POSITION(iy)
        neg
        ld      l,a
        ld      h,#0
        jr      nz,.cpmfs_prepare_room_ready
        inc     h                       ; 256 bytes
.cpmfs_prepare_room_ready:
        ld      de,(CPMFS_TMP_REMAINING)
        ld      a,d
        cp      h
        jr      c,.cpmfs_prepare_use_remaining
        jr      nz,.cpmfs_prepare_store
        ld      a,e
        cp      l
        jr      nc,.cpmfs_prepare_store
.cpmfs_prepare_use_remaining:
        ex      de,hl
.cpmfs_prepare_store:
        ld      (CPMFS_TMP_CHUNK),hl
        ret

        ;; Cap read chunk to size-position.
.cpmfs_cap_chunk_to_eof_iy:
        ld      a,FILE_SIZE_BYTES(iy)
        sub     FILE_POSITION(iy)
        ld      e,a
        ld      a,FILE_SIZE_BYTES + 1(iy)
        sbc     a,FILE_POSITION + 1(iy)
        ld      d,a
        ld      a,FILE_SIZE_BYTES + 2(iy)
        sbc     a,FILE_POSITION + 2(iy)
        ld      c,a
        ld      a,FILE_SIZE_BYTES + 3(iy)
        sbc     a,FILE_POSITION + 3(iy)
        or      c
        ret     nz
        ld      hl,(CPMFS_TMP_CHUNK)
        ld      a,d
        cp      h
        jr      c,.cpmfs_cap_chunk_store
        ret     nz
        ld      a,e
        cp      l
        ret     nc
.cpmfs_cap_chunk_store:
        ex      de,hl
        ld      (CPMFS_TMP_CHUNK),hl
        ret

        ;; Update request pointer/count/transferred after one chunk.
.cpmfs_account_chunk:
        ld      hl,(CPMFS_TMP_POINTER)
        ld      de,(CPMFS_TMP_CHUNK)
        add     hl,de
        ld      (CPMFS_TMP_POINTER),hl
        ld      hl,(CPMFS_TMP_REMAINING)
        or      a
        sbc     hl,de
        ld      (CPMFS_TMP_REMAINING),hl
        ld      hl,(CPMFS_TMP_TRANSFERRED)
        add     hl,de
        ld      (CPMFS_TMP_TRANSFERRED),hl
        ret

        ;; Load the current sector into a read cache.
.cpmfs_load_read_cache_iy:
        call    .cpmfs_position_sector_iy
        push    hl
        xor     a
        call    __cpmfs_map_sector_iy
        pop     hl
        ret     nz
        push    hl
        push    de
        call    __cpmfs_node_dma_iy
        pop     de
        push    de
        ld      a,FILE_DRIVE(iy)
        call    __cpmfs_read_sector
        pop     de
        pop     hl
        ret     nz
        call    .cpmfs_install_cache_key_iy
        xor     a
        ret

        ;; ------------------------------------------------------------
        ;; read
_read::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    iy
        ld      (CPMFS_TMP_FD),hl
        ld      (CPMFS_TMP_POINTER),de
        ld      c,4(ix)
        ld      b,5(ix)
        ld      (CPMFS_TMP_REMAINING),bc
        ld      hl,#0
        ld      (CPMFS_TMP_TRANSFERRED),hl
        ld      hl,(CPMFS_TMP_FD)
        call    __cpmfs_find_fd
        ld      a,h
        or      l
        jp      z,.cpmfs_read_bad_fd
        push    hl
        pop     iy
        ld      a,FILE_FLAGS(iy)
        and     #O_ACCMODE_VALUE
        cp      #O_WRONLY_VALUE
        jp      z,.cpmfs_read_access_fail
        ld      hl,(CPMFS_TMP_REMAINING)
        ld      a,h
        or      l
        jr      z,.cpmfs_read_loop
        ld      hl,(CPMFS_TMP_POINTER)
        ld      a,h
        or      l
        jp      z,.cpmfs_read_argument_fail
.cpmfs_read_loop:
        ld      hl,(CPMFS_TMP_REMAINING)
        ld      a,h
        or      l
        jr      z,.cpmfs_read_done
        call    .cpmfs_compare_position_size_iy
        jr      nc,.cpmfs_read_done
        call    .cpmfs_position_sector_iy
        call    .cpmfs_cache_sector_matches_iy
        call    nz,.cpmfs_load_read_cache_iy
        or      a
        jp      nz,.cpmfs_read_io_fail
        call    .cpmfs_prepare_chunk_iy
        call    .cpmfs_cap_chunk_to_eof_iy
        call    __cpmfs_node_dma_iy
        ld      e,FILE_POSITION(iy)
        ld      d,#0
        add     hl,de
        ld      de,(CPMFS_TMP_POINTER)
        ld      bc,(CPMFS_TMP_CHUNK)
        ldir
        call    .cpmfs_advance_position_iy
        call    .cpmfs_account_chunk
        jr      .cpmfs_read_loop
.cpmfs_read_done:
        xor     a
        call    __cpmfs_set_error
        ld      de,(CPMFS_TMP_TRANSFERRED)
        pop     iy
        pop     ix
        jp      .cpmfs_rw_return
.cpmfs_read_bad_fd:
        ld      a,#CPMFS_ERR_BAD_FD
        jr      .cpmfs_read_fail
.cpmfs_read_access_fail:
        ld      a,#CPMFS_ERR_ACCESS
        jr      .cpmfs_read_fail
.cpmfs_read_argument_fail:
        ld      a,#CPMFS_ERR_ARGUMENT
        jr      .cpmfs_read_fail
.cpmfs_read_io_fail:
        ld      hl,(CPMFS_TMP_TRANSFERRED)
        ld      a,h
        or      l
        jr      nz,.cpmfs_read_partial
        ld      a,#CPMFS_ERR_IO
.cpmfs_read_fail:
        call    __cpmfs_set_error
        ld      de,#0xffff
        pop     iy
        pop     ix
        jp      .cpmfs_rw_return
.cpmfs_read_partial:
        ld      de,(CPMFS_TMP_TRANSFERRED)
        pop     iy
        pop     ix
        jp      .cpmfs_rw_return

        ;; read/write each have one 16-bit stack parameter. xcc's register ABI
        ;; makes that final word callee-clean.
.cpmfs_rw_return:
        pop     hl                      ; return address
        pop     bc                      ; count
        jp      (hl)

        ;; Load/zero one sector for writing and install its cache key.
.cpmfs_load_write_cache_iy:
        call    .cpmfs_position_sector_iy
        push    hl
        ld      a,#1
        call    __cpmfs_map_sector_iy
        pop     hl
        ret     nz
        push    hl
        push    de
        ;; Read old data when sector_start < current size; otherwise zero it.
        ld      a,h
        cp      FILE_SIZE_BYTES + 2(iy)
        jr      c,.cpmfs_load_write_existing
        jr      nz,.cpmfs_load_write_zero
        ld      a,l
        cp      FILE_SIZE_BYTES + 1(iy)
        jr      c,.cpmfs_load_write_existing
        jr      nz,.cpmfs_load_write_zero
        ld      a,FILE_SIZE_BYTES(iy)
        or      a
        jr      nz,.cpmfs_load_write_existing
.cpmfs_load_write_zero:
        call    __cpmfs_node_dma_iy
        ld      bc,#256
.cpmfs_load_write_zero_loop:
        xor     a
        ld      (hl),a
        inc     hl
        dec     bc
        ld      a,b
        or      c
        jr      nz,.cpmfs_load_write_zero_loop
        pop     de
        pop     hl
        call    .cpmfs_install_cache_key_iy
        xor     a
        ret
.cpmfs_load_write_existing:
        call    __cpmfs_node_dma_iy
        pop     de
        push    de
        ld      a,FILE_DRIVE(iy)
        call    __cpmfs_read_sector
        pop     de
        pop     hl
        ret     nz
        call    .cpmfs_install_cache_key_iy
        xor     a
        ret

        ;; ------------------------------------------------------------
        ;; write
_write::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    iy
        ld      (CPMFS_TMP_FD),hl
        ld      (CPMFS_TMP_POINTER),de
        ld      c,4(ix)
        ld      b,5(ix)
        ld      (CPMFS_TMP_REMAINING),bc
        ld      hl,#0
        ld      (CPMFS_TMP_TRANSFERRED),hl
        ld      hl,(CPMFS_TMP_FD)
        call    __cpmfs_find_fd
        ld      a,h
        or      l
        jp      z,.cpmfs_write_bad_fd
        push    hl
        pop     iy
        ld      a,FILE_FLAGS(iy)
        and     #O_ACCMODE_VALUE
        jp      z,.cpmfs_write_access_fail
        ld      hl,(CPMFS_TMP_REMAINING)
        ld      a,h
        or      l
        jr      z,.cpmfs_write_pointer_valid
        ld      hl,(CPMFS_TMP_POINTER)
        ld      a,h
        or      l
        jp      z,.cpmfs_write_argument_fail
.cpmfs_write_pointer_valid:
        ld      a,FILE_FLAGS + 1(iy)
        and     #(O_APPEND_VALUE >> 8)
        call    nz,.cpmfs_position_to_end_iy
.cpmfs_write_loop:
        ld      hl,(CPMFS_TMP_REMAINING)
        ld      a,h
        or      l
        jr      z,.cpmfs_write_done
        call    .cpmfs_position_sector_iy
        call    .cpmfs_cache_sector_matches_iy
        call    nz,.cpmfs_load_write_cache_iy
        or      a
        jp      nz,.cpmfs_write_io_fail
        call    .cpmfs_prepare_chunk_iy
        call    __cpmfs_node_dma_iy
        ld      e,FILE_POSITION(iy)
        ld      d,#0
        add     hl,de
        ex      de,hl
        ld      hl,(CPMFS_TMP_POINTER)
        ld      bc,(CPMFS_TMP_CHUNK)
        ldir
        ld      a,#1
        ld      FILE_CACHE_DIRTY(iy),a
        call    .cpmfs_advance_position_iy
        call    .cpmfs_grow_size_iy
        call    .cpmfs_account_chunk
        jr      .cpmfs_write_loop
.cpmfs_write_done:
        xor     a
        call    __cpmfs_set_error
        ld      de,(CPMFS_TMP_TRANSFERRED)
        pop     iy
        pop     ix
        jp      .cpmfs_rw_return
.cpmfs_write_bad_fd:
        ld      a,#CPMFS_ERR_BAD_FD
        jr      .cpmfs_write_fail
.cpmfs_write_access_fail:
        ld      a,#CPMFS_ERR_ACCESS
        jr      .cpmfs_write_fail
.cpmfs_write_argument_fail:
        ld      a,#CPMFS_ERR_ARGUMENT
        jr      .cpmfs_write_fail
.cpmfs_write_io_fail:
        ld      hl,(CPMFS_TMP_TRANSFERRED)
        ld      a,h
        or      l
        jr      nz,.cpmfs_write_partial
        ld      a,#CPMFS_ERR_IO
.cpmfs_write_fail:
        call    __cpmfs_set_error
        ld      de,#0xffff
        pop     iy
        pop     ix
        jp      .cpmfs_rw_return
.cpmfs_write_partial:
        ld      de,(CPMFS_TMP_TRANSFERRED)
        pop     iy
        pop     ix
        jp      .cpmfs_rw_return

        ;; ------------------------------------------------------------
        ;; fcntl
_fcntl::
        push    iy
        push    de
        call    __cpmfs_find_fd
        pop     de
        ld      a,h
        or      l
        jr      z,.cpmfs_fcntl_bad
        push    hl
        pop     iy
        ld      a,d
        or      a
        jr      nz,.cpmfs_fcntl_sync_test
        ld      a,e
        cp      #F_GETFL_VALUE
        jr      nz,.cpmfs_fcntl_sync_test
        ld      e,FILE_FLAGS(iy)
        ld      d,FILE_FLAGS + 1(iy)
        pop     iy
        ret
.cpmfs_fcntl_sync_test:
        ld      hl,#CPMFS_F_SYNC_VALUE
        or      a
        sbc     hl,de
        jr      nz,.cpmfs_fcntl_bad
        call    __cpmfs_flush_iy
        jr      nz,.cpmfs_fcntl_io
        ld      a,FILE_FLAGS(iy)
        and     #O_ACCMODE_VALUE
        call    nz,__cpmfs_update_directory_iy
        jr      nz,.cpmfs_fcntl_io
        ld      de,#0
        pop     iy
        ret
.cpmfs_fcntl_bad:
        ld      a,#CPMFS_ERR_BAD_FD
        jr      .cpmfs_fcntl_fail
.cpmfs_fcntl_io:
        ld      a,#CPMFS_ERR_IO
.cpmfs_fcntl_fail:
        call    __cpmfs_set_error
        ld      de,#0xffff
        pop     iy
        ret

        ;; ------------------------------------------------------------
        ;; fstat
_fstat::
        push    iy
        ld      (CPMFS_TMP_POINTER),de
        call    __cpmfs_find_fd
        ld      a,h
        or      l
        jr      z,.cpmfs_fstat_bad_fd
        ld      de,(CPMFS_TMP_POINTER)
        ld      a,d
        or      e
        jr      z,.cpmfs_fstat_argument
        push    hl
        pop     iy
        ld      hl,(CPMFS_TMP_POINTER)
        ld      a,FILE_DRIVE(iy)
        ld      (hl),a                  ; st_dev
        inc     hl
        ld      (hl),#0
        inc     hl
        ld      a,FILE_FD(iy)           ; st_ino (descriptor-derived)
        ld      (hl),a
        inc     hl
        ld      a,FILE_FD + 1(iy)
        ld      (hl),a
        inc     hl
        ld      (hl),#0
        inc     hl
        ld      (hl),#0
        inc     hl
        ld      de,#S_IFREG_PERMISSIONS
        ld      (hl),e                  ; st_mode
        inc     hl
        ld      (hl),d
        inc     hl
        ld      (hl),#1                 ; st_nlink
        inc     hl
        ld      (hl),#0
        inc     hl
        ld      a,FILE_SIZE_BYTES(iy)   ; st_size
        ld      (hl),a
        inc     hl
        ld      a,FILE_SIZE_BYTES + 1(iy)
        ld      (hl),a
        inc     hl
        ld      a,FILE_SIZE_BYTES + 2(iy)
        ld      (hl),a
        inc     hl
        ld      a,FILE_SIZE_BYTES + 3(iy)
        ld      (hl),a
        ld      de,#0
        pop     iy
        ret
.cpmfs_fstat_bad_fd:
        ld      a,#CPMFS_ERR_BAD_FD
        jr      .cpmfs_fstat_fail
.cpmfs_fstat_argument:
        ld      a,#CPMFS_ERR_ARGUMENT
.cpmfs_fstat_fail:
        call    __cpmfs_set_error
        ld      de,#0xffff
        pop     iy
        ret

        ;; ------------------------------------------------------------
        ;; lseek
_lseek::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    iy
        ld      (CPMFS_TMP_FD),hl
        ld      hl,(CPMFS_TMP_FD)
        call    __cpmfs_find_fd
        ld      a,h
        or      l
        jp      z,.cpmfs_lseek_bad
        push    hl
        pop     iy
        call    __cpmfs_flush_iy
        jp      nz,.cpmfs_lseek_io
        ld      a,9(ix)
        or      a
        jp      nz,.cpmfs_lseek_range
        ld      a,8(ix)
        cp      #SEEK_SET_VALUE
        jr      z,.cpmfs_lseek_zero_base
        cp      #SEEK_CUR_VALUE
        jr      z,.cpmfs_lseek_current_base
        cp      #SEEK_END_VALUE
        jp      nz,.cpmfs_lseek_range
        ld      a,FILE_SIZE_BYTES(iy)
        ld      (CPMFS_TMP_VALUE),a
        ld      a,FILE_SIZE_BYTES + 1(iy)
        ld      (CPMFS_TMP_VALUE + 1),a
        ld      a,FILE_SIZE_BYTES + 2(iy)
        ld      (CPMFS_TMP_VALUE + 2),a
        ld      a,FILE_SIZE_BYTES + 3(iy)
        ld      (CPMFS_TMP_VALUE + 3),a
        jr      .cpmfs_lseek_add
.cpmfs_lseek_current_base:
        ld      a,FILE_POSITION(iy)
        ld      (CPMFS_TMP_VALUE),a
        ld      a,FILE_POSITION + 1(iy)
        ld      (CPMFS_TMP_VALUE + 1),a
        ld      a,FILE_POSITION + 2(iy)
        ld      (CPMFS_TMP_VALUE + 2),a
        ld      a,FILE_POSITION + 3(iy)
        ld      (CPMFS_TMP_VALUE + 3),a
        jr      .cpmfs_lseek_add
.cpmfs_lseek_zero_base:
        xor     a
        ld      (CPMFS_TMP_VALUE),a
        ld      (CPMFS_TMP_VALUE + 1),a
        ld      (CPMFS_TMP_VALUE + 2),a
        ld      (CPMFS_TMP_VALUE + 3),a
.cpmfs_lseek_add:
        ld      a,7(ix)
        and     #0x80
        jr      nz,.cpmfs_lseek_subtract
        ld      a,(CPMFS_TMP_VALUE)
        add     a,4(ix)
        ld      (CPMFS_TMP_VALUE),a
        ld      a,(CPMFS_TMP_VALUE + 1)
        adc     a,5(ix)
        ld      (CPMFS_TMP_VALUE + 1),a
        ld      a,(CPMFS_TMP_VALUE + 2)
        adc     a,6(ix)
        ld      (CPMFS_TMP_VALUE + 2),a
        ld      a,(CPMFS_TMP_VALUE + 3)
        adc     a,7(ix)
        ld      (CPMFS_TMP_VALUE + 3),a
        jp      c,.cpmfs_lseek_range
        jr      .cpmfs_lseek_check_size
.cpmfs_lseek_subtract:
        ;; Convert the negative two's-complement offset to its magnitude.
        ld      a,4(ix)
        cpl
        ld      (CPMFS_TMP_VALUE2),a
        ld      a,5(ix)
        cpl
        ld      (CPMFS_TMP_VALUE2 + 1),a
        ld      a,6(ix)
        cpl
        ld      (CPMFS_TMP_VALUE2 + 2),a
        ld      a,7(ix)
        cpl
        ld      (CPMFS_TMP_VALUE2 + 3),a
        ld      hl,#CPMFS_TMP_VALUE2
        inc     (hl)
        jr      nz,.cpmfs_lseek_subtract_ready
        inc     hl
        inc     (hl)
        jr      nz,.cpmfs_lseek_subtract_ready
        inc     hl
        inc     (hl)
        jr      nz,.cpmfs_lseek_subtract_ready
        inc     hl
        inc     (hl)
.cpmfs_lseek_subtract_ready:
        ld      a,(CPMFS_TMP_VALUE)
        ld      b,a
        ld      a,(CPMFS_TMP_VALUE2)
        ld      c,a
        ld      a,b
        sub     c
        ld      (CPMFS_TMP_VALUE),a
        ld      a,(CPMFS_TMP_VALUE + 1)
        ld      b,a
        ld      a,(CPMFS_TMP_VALUE2 + 1)
        ld      c,a
        ld      a,b
        sbc     a,c
        ld      (CPMFS_TMP_VALUE + 1),a
        ld      a,(CPMFS_TMP_VALUE + 2)
        ld      b,a
        ld      a,(CPMFS_TMP_VALUE2 + 2)
        ld      c,a
        ld      a,b
        sbc     a,c
        ld      (CPMFS_TMP_VALUE + 2),a
        ld      a,(CPMFS_TMP_VALUE + 3)
        ld      b,a
        ld      a,(CPMFS_TMP_VALUE2 + 3)
        ld      c,a
        ld      a,b
        sbc     a,c
        ld      (CPMFS_TMP_VALUE + 3),a
        jp      c,.cpmfs_lseek_range
.cpmfs_lseek_check_size:
        ;; Require new position <= file size (CP/M has no sparse files).
        ld      hl,#CPMFS_TMP_VALUE + 3
        push    iy
        pop     de
        ld      bc,#(FILE_SIZE_BYTES + 3)
        ex      de,hl
        add     hl,bc
        ex      de,hl
        ld      b,#4
.cpmfs_lseek_compare:
        ld      a,(de)
        cp      (hl)
        jp      c,.cpmfs_lseek_range
        jr      nz,.cpmfs_lseek_store
        dec     de
        dec     hl
        djnz    .cpmfs_lseek_compare
.cpmfs_lseek_store:
        ld      a,(CPMFS_TMP_VALUE)
        ld      FILE_POSITION(iy),a
        ld      e,a
        ld      a,(CPMFS_TMP_VALUE + 1)
        ld      FILE_POSITION + 1(iy),a
        ld      d,a
        ld      a,(CPMFS_TMP_VALUE + 2)
        ld      FILE_POSITION + 2(iy),a
        ld      l,a
        ld      a,(CPMFS_TMP_VALUE + 3)
        ld      FILE_POSITION + 3(iy),a
        ld      h,a
        xor     a
        ld      FILE_CACHE_VALID(iy),a
        pop     iy
        pop     ix
        ret
.cpmfs_lseek_bad:
        ld      a,#CPMFS_ERR_BAD_FD
        jr      .cpmfs_lseek_fail
.cpmfs_lseek_io:
        ld      a,#CPMFS_ERR_IO
        jr      .cpmfs_lseek_fail
.cpmfs_lseek_range:
        ld      a,#CPMFS_ERR_RANGE
.cpmfs_lseek_fail:
        call    __cpmfs_set_error
        ld      de,#0xffff
        ld      hl,#0xffff
        pop     iy
        pop     ix
        ret

        ;; ------------------------------------------------------------
        ;; chdir
_chdir::
        push    hl
        call    _cpmfs_init
        ld      a,d
        and     e
        inc     a
        pop     hl
        jr      z,.cpmfs_chdir_init_fail
        call    __cpmfs_parse_directory
        or      a
        jr      nz,.cpmfs_chdir_fail
        ld      a,b
        ld      (CPMFS_STATE_DRIVE),a
        ld      a,c
        ld      (CPMFS_STATE_AREA),a
        xor     a
        call    __cpmfs_set_error
        ld      de,#0
        ret
.cpmfs_chdir_fail:
        ld      a,#CPMFS_ERR_PATH
        call    __cpmfs_set_error
.cpmfs_chdir_init_fail:
        ld      de,#0xffff
        ret

        ;; ------------------------------------------------------------
        ;; stat
_stat::
        ld      a,d
        or      e
        jr      z,.cpmfs_stat_argument
        push    ix
        push    iy
        push    de
        ld      de,#0
        call    _open
        ld      a,d
        and     e
        inc     a
        jr      z,.cpmfs_stat_open_fail
        ld      (CPMFS_TMP_FD),de
        pop     bc
        push    bc
        ld      h,d
        ld      l,e
        ld      d,b
        ld      e,c
        call    _fstat
        push    de
        ld      hl,(CPMFS_TMP_FD)
        call    _close
        pop     de
        pop     bc
        pop     iy
        pop     ix
        ret
.cpmfs_stat_open_fail:
        pop     bc
        ld      de,#0xffff
        pop     iy
        pop     ix
        ret
.cpmfs_stat_argument:
        ld      a,#CPMFS_ERR_ARGUMENT
        call    __cpmfs_set_error
        ld      de,#0xffff
        ret
