        .ifndef YOS_CPC_AMSDOS
        .equ YOS_CPC_AMSDOS,0
        .endif
        ;; YOS ABI-1 filesystem entry points backed by baremetal libcpmfs.
        ;; MIT License (see LICENSE), Copyright (C) 2026 Tomaz Stih.
        .module partner_yos_files
        .include "cpmfs.inc"
        .globl __native_fs_open, __native_fs_close, __native_fs_read, __native_fs_write
        .globl __native_fs_seek, __native_fs_stat, __native_fs_fstat
        .globl __native_fs_unlink, __native_fs_rename, __native_fs_errno, __native_fs_reap
        .globl __native_opendir, __native_readdir, __native_rewinddir, __native_closedir
        .globl __native_enum_disks, __native_current, __partner_path
        .globl _open, _close, _read, _write, _lseek, _stat, _fstat, _rename
        .globl __cpmfs_read_sector, __cpmfs_set_error
        .globl _opendir, _readdir, _rewinddir, _closedir
        .globl __cpmfs_find_fd, __cpmfs_delete_iy
        .globl __partner_fs_track, __partner_fs_untrack
        .equ __native_fs_errno,CPMFS_STATE_ERRNO
        ;; Partner extends libcpmfs DIR by 36 bytes: eight 32-bit sizes,
        ;; the cached sector, and a filesystem mutation epoch.
        .equ DIRECTORY_SIZES,280
        .equ DIRECTORY_SIZE_SECTOR,312
        .area _BSS
.handles: .ds 96                     ; 16 {handle, owner, kind, active}
.kind: .ds 1
.oldpath: .ds 32
.status: .ds 14
.dir_epoch: .ds 2
.sizes_dma: .ds 256
.sizes_lba: .ds 2
.sizes_sector_key: .ds 2
.sizes_slot: .ds 1
.sizes_bytes: .ds 4
        .area _CODE
__native_fs_open::
        call .directory_changed
        call __partner_path
        or a
        jp nz,.error
        bit 3,d                         ; O_EXCL only has meaning with O_CREAT
        jr z,.open_ready
        bit 0,d
        jr z,.open_ready
        push hl
        push de
        ld de,#.status
        call _stat
        ld a,d
        or e
        pop de
        pop hl
        jr nz,.open_ready
        ld de,#17                       ; EEXIST, do not truncate existing file
        ld (__native_fs_errno),de
        jp .error
.open_ready:
        call _open
        ld a,d
        and e
        inc a
        ret z
        xor a
        jp __partner_fs_track
__native_opendir::
        call __partner_path
        or a
        jr nz,.null
        call _opendir
        ld a,d
        or e
        ret z
        ld a,#1
        jp __partner_fs_track
.null:
        ld de,#0
        ret
__partner_fs_track::
        push ix
        ld (.kind),a
        ld ix,#.handles
        ld b,#16
.find:
        bit 0,5(ix)
        jr z,.slot
        push de
        ld de,#6
        add ix,de
        pop de
        djnz .find
        ex de,hl
        ld a,(.kind)
        or a
        jr nz,.track_dir_fail
        call _close
        pop ix
        jp .error
.track_dir_fail:
        call _closedir
        pop ix
        jp .null
.slot:
        ld 0(ix),e
        ld 1(ix),d
        ld hl,(__native_current)
        ld 2(ix),l
        ld 3(ix),h
        ld a,(.kind)
        ld 4(ix),a
        ld 5(ix),#1
        pop ix
        ret
__native_fs_close::
        call .directory_changed
        xor a
        call __partner_fs_untrack
        jp _close
__native_closedir::
        ld a,#1
        call __partner_fs_untrack
        jp _closedir
__partner_fs_untrack::
        push ix
        push bc
        push de
        ld c,a
        ld ix,#.handles
        ld b,#16
.untrack_loop:
        ld a,4(ix)
        cp c
        jr nz,.untrack_next
        ld a,0(ix)
        cp l
        jr nz,.untrack_next
        ld a,1(ix)
        cp h
        jr nz,.untrack_next
        ld 5(ix),#0
.untrack_next:
        ld de,#6
        add ix,de
        djnz .untrack_loop
        pop de
        pop bc
        pop ix
        ret
__native_fs_reap::
        push ix
        push iy
        push ix
        pop iy
        ld ix,#.handles
        ld b,#16
.reap:
        push bc
        bit 0,5(ix)
        jr z,.reap_next
        push iy
        pop de
        ld a,2(ix)
        cp e
        jr nz,.reap_next
        ld a,3(ix)
        cp d
        jr nz,.reap_next
        ld 5(ix),#0
        ld l,0(ix)
        ld h,1(ix)
        ld a,4(ix)
        or a
        jr nz,.reap_dir
        call _close
        jr .reap_next
.reap_dir:
        call _closedir
.reap_next:
        ld de,#6
        add ix,de
        pop bc
        djnz .reap
        pop iy
        pop ix
        ret
__native_fs_read::
        ; YOS ABI keeps count caller-owned; libcpmfs consumes its count.
        pop bc
        pop af
        push af
        push bc
        push af
        call _read
        ret
__native_fs_write::
        call .directory_changed
        ; YOS ABI keeps count caller-owned; libcpmfs consumes its count.
        pop bc
        pop af
        push af
        push bc
        push af
        call _write
        ret
__native_fs_seek::
        jp _lseek
__native_fs_fstat::
        jp _fstat
__native_fs_stat::
        call __partner_path
        or a
        jp nz,.error
        jp _stat
__native_fs_rename::
        call .directory_changed
        push de
        call __partner_path
        or a
        jr nz,.rename_fail
        ld de,#.oldpath
        ld bc,#32
        ldir
        pop hl
        call __partner_path
        or a
        jp nz,.error
        ex de,hl
        ld hl,#.oldpath
        jp _rename
.rename_fail:
        pop de
.error:
        ld de,#0xffff
        ret
__native_fs_unlink::
        call .directory_changed
        call __partner_path
        or a
        jp nz,.error
        ld de,#0
        call _open
        ld a,d
        and e
        inc a
        ret z
        push iy
        push de
        ex de,hl
        call __cpmfs_find_fd
        push hl
        pop iy
        call __cpmfs_delete_iy
        pop hl
        push af
        call _close
        pop af
        pop iy
        or a
        jp nz,.error
        ld de,#0
        ret
.directory_changed:
        push hl
        ld hl,(.dir_epoch)
        inc hl
        ld (.dir_epoch),hl
        pop hl
        ret
__native_rewinddir::
        jp _rewinddir
__native_readdir::
        call _readdir
        ld a,d
        or e
        ret z
        push ix
        push iy
        push de
        pop ix                         ; CP/M short-name result at DIR+8
        ld hl,#DIRECTORY_PUBLIC_ENTRY-DIRECTORY_RESULT
        add hl,de
        push hl
        pop iy                         ; this stream owns the full YOS dirent
        push hl
        pop de
        inc de
        ld bc,#23                      ; header, DOS alias, empty long name
        ld (hl),#0
        ldir
        ld 8(iy),#8                    ; DT_REG
        ld hl,#10
        push iy
        pop de
        add hl,de
        ex de,hl
        push ix
        pop hl
        ld bc,#13
        ldir
        call .dir_sizes
        jr c,.dir_io_error
        push iy
        pop de
        inc de
        inc de
        inc de
        inc de
        ld bc,#4
        ldir
.dir_result:
        push iy
        pop de
        pop iy
        pop ix
        ret
.dir_io_error:
        ld a,#CPMFS_ERR_IO
        call __cpmfs_set_error
        pop iy
        pop ix
        ld de,#0
        ret
        ;; Cache eight file lengths per directory sector. A single extent
        ;; scan fills them all, including sparse files and CP/M byte counts.
        ;; IX points to libcpmfs's public name (DIR+8). CF means I/O failure.
.dir_sizes:
        push ix
        push iy
        push ix
        pop hl
        ld de,#-DIRECTORY_RESULT
        add hl,de
        push hl
        pop ix
        ld l,DIRECTORY_LBA(ix)
        ld h,DIRECTORY_LBA+1(ix)
        ld a,DIRECTORY_INDEX(ix)
        or a
        jr nz,.sizes_sector
        dec hl                          ; index already advanced on return
.sizes_sector:
        ld (.sizes_sector_key),hl
        ex de,hl
        push ix
        pop hl
        ld bc,#DIRECTORY_SIZE_SECTOR
        add hl,bc
        ld a,(hl)
        cp e
        jr nz,.sizes_fill
        inc hl
        ld a,(hl)
        cp d
        jr nz,.sizes_fill
        inc hl
        ld de,(.dir_epoch)
        ld a,(hl)
        cp e
        jr nz,.sizes_fill
        inc hl
        ld a,(hl)
        cp d
        jp z,.sizes_result
.sizes_fill:
        push ix
        pop hl
        ld de,#DIRECTORY_SIZES
        add hl,de
        push hl
        pop de
        inc de
        ld (hl),#0
        ld bc,#35                       ; clear sizes and invalidate key
        ldir
        ld a,DIRECTORY_DRIVE(ix)
        call __cpmfs_geometry
        ld hl,(CPMFS_G_DIR)
        ld (.sizes_lba),hl
.sizes_read:
        ld de,(.sizes_lba)
        ld hl,#.sizes_dma
        ld a,DIRECTORY_DRIVE(ix)
        call __cpmfs_read_sector
        or a
        jp nz,.sizes_fail
        ld iy,#.sizes_dma
        ld b,#8
.sizes_entry:
        push bc
        ld a,DIRENT_USER(iy)
        cp DIRECTORY_AREA(ix)
        jr nz,.sizes_next
        call .sizes_value
        push ix
        pop hl
        ld de,#DIRECTORY_DMA
        add hl,de
        xor a
        ld (.sizes_slot),a
.sizes_match:
        push hl
        ld a,(hl)
        cp DIRECTORY_AREA(ix)
        jr nz,.sizes_mismatch
        inc hl
        push iy
        pop de
        inc de
        ld b,#11
.sizes_name:
        ld a,(de)
        and #127
        ld c,a
        ld a,(hl)
        and #127
        cp c
        jr nz,.sizes_mismatch
        inc hl
        inc de
        djnz .sizes_name
        call .sizes_keep
.sizes_mismatch:
        pop hl
        ld de,#32
        add hl,de
        ld a,(.sizes_slot)
        inc a
        ld (.sizes_slot),a
        cp #8
        jr c,.sizes_match
.sizes_next:
        ld de,#32
        add iy,de
        pop bc
        djnz .sizes_entry
        ld hl,(.sizes_lba)
        inc hl
        ld (.sizes_lba),hl
        ld de,(CPMFS_G_END)
        or a
        sbc hl,de
        jp c,.sizes_read
        push ix
        pop hl
        ld de,#DIRECTORY_SIZE_SECTOR
        add hl,de
        ld de,(.sizes_sector_key)
        ld (hl),e
        inc hl
        ld (hl),d
        inc hl
        ld de,(.dir_epoch)
        ld (hl),e
        inc hl
        ld (hl),d
.sizes_result:
        ld a,DIRECTORY_INDEX(ix)
        dec a
        and #7
        add a,a
        add a,a
        ld e,a
        ld d,#0
        push ix
        pop hl
        add hl,de
        ld de,#DIRECTORY_SIZES
        add hl,de
        or a
        pop iy
        pop ix
        ret
.sizes_fail:
        scf
        pop iy
        pop ix
        ret
        ;; End offset = ((S2*32 + EX)*128 + RC)*128, adjusted by BC.
.sizes_value:
        ld a,DIRENT_S2(iy)
        and #63
        ld l,a
        ld h,#0
        ld b,#5
.sizes_extent_shift:
        add hl,hl
        djnz .sizes_extent_shift
        ld a,DIRENT_EX(iy)
        and #31
        or l
        ld l,a
        ld de,#0
        call .sizes_shift7
        ld c,DIRENT_RC(iy)
        ld b,#0
        add hl,bc
        jr nc,.sizes_records
        inc de
.sizes_records:
        call .sizes_shift7
        bit 7,6(iy)
        jr z,.sizes_store
        ld a,DIRENT_RC(iy)
        or a
        jr z,.sizes_store
        ld a,DIRENT_BC(iy)
        or a
        jr z,.sizes_store
        cp #128
        jr nc,.sizes_store
        ld c,a
        ld b,#0
        push bc
        ld bc,#128
        or a
        sbc hl,bc
        jr nc,.sizes_add_bytes
        dec de
.sizes_add_bytes:
        pop bc
        add hl,bc
        jr nc,.sizes_store
        inc de
.sizes_store:
        ld (.sizes_bytes),hl
        ld (.sizes_bytes+2),de
        ret
.sizes_shift7:
        ld b,#7
.sizes_shift:
        add hl,hl
        rl e
        rl d
        djnz .sizes_shift
        ret
.sizes_keep:
        ld a,(.sizes_slot)
        add a,a
        add a,a
        ld e,a
        ld d,#0
        push ix
        pop hl
        add hl,de
        ld de,#DIRECTORY_SIZES+3
        add hl,de
        ld de,#.sizes_bytes+3
        ld b,#4
.sizes_compare:
        ld a,(de)
        cp (hl)
        ret c
        jr nz,.sizes_larger
        dec hl
        dec de
        djnz .sizes_compare
        ret
.sizes_larger:
        ;; B is the number of low bytes including the differing byte.
        ld a,(de)
        ld (hl),a
        dec hl
        dec de
        djnz .sizes_larger
        ret

__native_enum_disks::
        ld a,d
        or e
        ret z
        ld bc,#6
        ld (hl),#0x40                 ; A: floppy
        call .descriptor
        .if YOS_CPC_AMSDOS
        .else
        dec de
        ld a,d
        or e
        .endif
        ld de,#1
        .if YOS_CPC_AMSDOS
        .else
        ret z
        ;; An unattached SASI controller reads FFh. Do not invent a B: disk
        ;; when the machine booted only from its floppy.
        in a,(0x10)
        cp #255
        ret z
        ld (hl),#0x48                 ; B: hard disk
        call .descriptor
        ld de,#2
        .endif
        ret
.descriptor:
        inc hl
        ld (hl),#1
        inc hl
        ld (hl),#0
        inc hl
        ld (hl),#0
        inc hl
        ld (hl),#0
        inc hl
        ld (hl),#0
        inc hl
        ret
