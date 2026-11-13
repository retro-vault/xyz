        .ifndef YOS_CPC_AMSDOS
        .equ YOS_CPC_AMSDOS,0
        .endif
        ;; cpmfs_dir.s
        ;;
        ;; CP/M user-area directory streams. Every stream is a shared-heap
        ;; allocation containing its own 256-byte raw-directory DMA buffer.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-08-12   tstih

        .module cpmfs_dir
        .optsdcc -mz80 sdcccall(1)

        .include "cpmfs.inc"

        .globl  _opendir
        .globl  _readdir
        .globl  _closedir
        .globl  _rewinddir
        .globl  _cpmfs_init
        .globl  __cpmfs_parse_directory
        .globl  __cpmfs_set_error
        .globl  __cpmfs_read_sector
        .globl  _smalloc
        .globl  _sfree

        .area   _CODE

        ;; Return HL = IY + DIRECTORY_DMA.
.cpmfs_directory_dma_iy:
        push    de
        push    iy
        pop     hl
        ld      de,#DIRECTORY_DMA
        add     hl,de
        pop     de
        ret

        ;; ------------------------------------------------------------
        ;; opendir
        ;; Open one device/user-area pair and allocate a private DMA buffer.
_opendir::
        push    iy
        push    hl
        call    _cpmfs_init
        ld      a,d
        and     e
        inc     a
        pop     hl
        jr      z,.cpmfs_opendir_memory_fail
        call    __cpmfs_parse_directory
        or      a
        jr      nz,.cpmfs_opendir_path_fail
        ld      a,b
        cp      #5
        jr      nc,.cpmfs_opendir_device_fail
        ld      (CPMFS_TMP_CREATE),a
        ld      a,c
        ld      (CPMFS_TMP_FOUND),a
        ld      hl,#DIRECTORY_SIZE
        call    _smalloc
        ld      a,d
        or      e
        jr      z,.cpmfs_opendir_memory_fail
        push    de
        pop     iy
        push    iy
        pop     hl
        ld      bc,#DIRECTORY_SIZE
.cpmfs_opendir_zero_loop:
        xor     a
        ld      (hl),a
        inc     hl
        dec     bc
        ld      a,b
        or      c
        jr      nz,.cpmfs_opendir_zero_loop
        .if YOS_CPC_AMSDOS
        push iy
        pop hl
        ld de,#312
        add hl,de
        ld (hl),#255
        inc hl
        ld (hl),#255
        .endif
        ld      DIRECTORY_MAGIC(iy),#(DIRECTORY_MAGIC_VALUE & 0xff)
        ld      DIRECTORY_MAGIC + 1(iy),#(DIRECTORY_MAGIC_VALUE >> 8)
        ld      a,(CPMFS_TMP_CREATE)
        ld      DIRECTORY_DRIVE(iy),a
        ld      a,(CPMFS_TMP_FOUND)
        ld      DIRECTORY_AREA(iy),a
        ld      a,DIRECTORY_DRIVE(iy)
        call    __cpmfs_geometry
        ld      hl,(CPMFS_G_DIR)
        ld      DIRECTORY_LBA(iy),l
        ld      DIRECTORY_LBA + 1(iy),h
        xor     a
        call    __cpmfs_set_error
        push    iy
        pop     de
        pop     iy
        ret
.cpmfs_opendir_path_fail:
        ld      a,#CPMFS_ERR_PATH
        jr      .cpmfs_opendir_fail
.cpmfs_opendir_device_fail:
        ld      a,#CPMFS_ERR_DEVICE
        jr      .cpmfs_opendir_fail
.cpmfs_opendir_memory_fail:
        ld      a,#CPMFS_ERR_MEMORY
.cpmfs_opendir_fail:
        call    __cpmfs_set_error
        ld      de,#0
        pop     iy
        ret

        ;; Validate the directory pointer in HL and return it in IY.
        ;; Z means valid, NZ invalid.
.cpmfs_directory_validate:
        ld      a,h
        or      l
        jr      z,.cpmfs_directory_invalid
        push    hl
        pop     iy
        ld      a,DIRECTORY_MAGIC(iy)
        cp      #(DIRECTORY_MAGIC_VALUE & 0xff)
        jr      nz,.cpmfs_directory_invalid
        ld      a,DIRECTORY_MAGIC + 1(iy)
        cp      #(DIRECTORY_MAGIC_VALUE >> 8)
        ret
.cpmfs_directory_invalid:
        ld      a,#1
        or      a
        ret

        ;; ------------------------------------------------------------
        ;; readdir
        ;; Return only extent-zero entries so a multi-extent CP/M file appears
        ;; exactly once in the stream.
_readdir::
        push    ix
        push    iy
        call    .cpmfs_directory_validate
        jp      nz,.cpmfs_readdir_bad
        ld      a,DIRECTORY_DRIVE(iy)
        call    __cpmfs_geometry
.cpmfs_readdir_loop:
        ld      e,DIRECTORY_LBA(iy)
        ld      d,DIRECTORY_LBA + 1(iy)
        ld      hl,(CPMFS_G_END)
        or      a
        sbc     hl,de
        jp      z,.cpmfs_readdir_end
        jp      c,.cpmfs_readdir_end
        ld      a,DIRECTORY_VALID(iy)
        or      a
        jr      nz,.cpmfs_readdir_have_sector
        push    de
        call    .cpmfs_directory_dma_iy
        pop     de
        ld      a,DIRECTORY_DRIVE(iy)
        call    __cpmfs_read_sector
        or      a
        jp      nz,.cpmfs_readdir_io
        ld      DIRECTORY_VALID(iy),#1
.cpmfs_readdir_have_sector:
        call    .cpmfs_directory_dma_iy
        ld      a,DIRECTORY_INDEX(iy)
        ld      e,a
        ld      d,#0
        ld      b,#5
.cpmfs_readdir_index_shift:
        sla     e
        rl      d
        djnz    .cpmfs_readdir_index_shift
        add     hl,de                  ; entry index * 32
        push    hl
        pop     ix
        ld      a,DIRECTORY_INDEX(iy)
        inc     a
        cp      #CPMFS_ENTRIES_PER_SECTOR
        jr      c,.cpmfs_readdir_store_index
        xor     a
        ld      DIRECTORY_VALID(iy),a
        ld      l,DIRECTORY_LBA(iy)
        ld      h,DIRECTORY_LBA + 1(iy)
        inc     hl
        ld      DIRECTORY_LBA(iy),l
        ld      DIRECTORY_LBA + 1(iy),h
.cpmfs_readdir_store_index:
        ld      DIRECTORY_INDEX(iy),a
        ld      a,DIRENT_USER(ix)
        cp      DIRECTORY_AREA(iy)
        jr      nz,.cpmfs_readdir_loop
        ld      a,(CPMFS_G_EXM)
        cpl
        and     #0x1f
        and     DIRENT_EX(ix)
        jr      nz,.cpmfs_readdir_loop
        ld      a,DIRENT_S2(ix)
        and     #0x3f
        jr      nz,.cpmfs_readdir_loop

        ;; Clear the public result and format its NUL-terminated 8.3 name.
        push    iy
        pop     hl
        ld      de,#DIRECTORY_RESULT
        add     hl,de
        push    hl
        ld      b,#13
.cpmfs_readdir_clear_result:
        ld      (hl),#0
        inc     hl
        djnz    .cpmfs_readdir_clear_result
        pop     de                      ; output cursor
        push    ix
        pop     hl
        inc     hl                      ; input name
        ld      b,#8
.cpmfs_readdir_name_loop:
        ld      a,(hl)
        and     #0x7f
        cp      #' '
        jr      z,.cpmfs_readdir_name_done
        ld      (de),a
        inc     de
        inc     hl
        djnz    .cpmfs_readdir_name_loop
.cpmfs_readdir_name_done:
        push    ix
        pop     hl
        ld      bc,#9
        add     hl,bc                   ; first extension byte
        ld      b,#3
        ld      c,#0
.cpmfs_readdir_extension_test:
        ld      a,(hl)
        and     #0x7f
        cp      #' '
        jr      nz,.cpmfs_readdir_have_extension
        inc     hl
        djnz    .cpmfs_readdir_extension_test
        jr      .cpmfs_readdir_result_ready
.cpmfs_readdir_have_extension:
        ld      a,#'.'
        ld      (de),a
        inc     de
        push    ix
        pop     hl
        ld      bc,#9
        add     hl,bc
        ld      b,#3
.cpmfs_readdir_extension_loop:
        ld      a,(hl)
        and     #0x7f
        cp      #' '
        jr      z,.cpmfs_readdir_result_ready
        ld      (de),a
        inc     de
        inc     hl
        djnz    .cpmfs_readdir_extension_loop
.cpmfs_readdir_result_ready:
        xor     a
        ld      (de),a
        ld      a,DIRECTORY_DRIVE(iy)
        ld      DIRECTORY_RESULT_DRIVE(iy),a
        ld      a,DIRECTORY_AREA(iy)
        ld      DIRECTORY_RESULT_AREA(iy),a
        xor     a
        call    __cpmfs_set_error
        push    iy
        pop     de
        ld      hl,#DIRECTORY_RESULT
        add     hl,de
        ex      de,hl
        pop     iy
        pop     ix
        ret
.cpmfs_readdir_end:
        xor     a
        call    __cpmfs_set_error
        ld      de,#0
        pop     iy
        pop     ix
        ret
.cpmfs_readdir_bad:
        ld      a,#CPMFS_ERR_ARGUMENT
        jr      .cpmfs_readdir_fail
.cpmfs_readdir_io:
        ld      a,#CPMFS_ERR_IO
.cpmfs_readdir_fail:
        call    __cpmfs_set_error
        ld      de,#0
        pop     iy
        pop     ix
        ret

        ;; ------------------------------------------------------------
        ;; closedir
_closedir::
        push    iy
        call    .cpmfs_directory_validate
        jr      nz,.cpmfs_closedir_bad
        xor     a
        ld      DIRECTORY_MAGIC(iy),a
        ld      DIRECTORY_MAGIC + 1(iy),a
        push    iy
        pop     hl
        call    _sfree
        xor     a
        call    __cpmfs_set_error
        ld      de,#0
        pop     iy
        ret
.cpmfs_closedir_bad:
        ld      a,#CPMFS_ERR_ARGUMENT
        call    __cpmfs_set_error
        ld      de,#0xffff
        pop     iy
        ret

        ;; ------------------------------------------------------------
        ;; rewinddir
_rewinddir::
        push    iy
        call    .cpmfs_directory_validate
        jr      nz,.cpmfs_rewinddir_bad
        ld      a,DIRECTORY_DRIVE(iy)
        call    __cpmfs_geometry
        ld      hl,(CPMFS_G_DIR)
        ld      DIRECTORY_LBA(iy),l
        ld      DIRECTORY_LBA + 1(iy),h
        xor     a
        ld      DIRECTORY_INDEX(iy),a
        ld      DIRECTORY_VALID(iy),a
        call    __cpmfs_set_error
        pop     iy
        ret
.cpmfs_rewinddir_bad:
        ld      a,#CPMFS_ERR_ARGUMENT
        call    __cpmfs_set_error
        pop     iy
        ret
