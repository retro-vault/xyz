        ;; Native NextZXOS file calls. Files embedded in the NEX are a read-only
        ;; overlay for system images; they get native handles E1..F0. Every other
        ;; request is passed to the native +3DOS API through native_calls.s.
        .module next_calls
        .optsdcc -mz80 sdcccall(1)
        .globl __next_fs_f_open, __next_fs_f_close, __next_fs_f_sync, __next_fs_f_read
        .globl __next_fs_f_write, __next_fs_f_seek, __next_fs_f_fgetpos, __next_fs_f_fstat
        .globl __next_fs_f_getcwd, __next_fs_f_chdir, __next_fs_f_mkdir, __next_fs_f_rmdir
        .globl __next_fs_f_stat, __next_fs_f_unlink, __next_fs_f_rename, __next_fs_f_opendir
        .globl __next_fs_f_readdir, __next_fs_f_rewinddir, __next_fs_disk_info
        .globl __next_fs_m_execcmd
        .globl __next_fs_real_f_open, __next_fs_real_f_close, __next_fs_real_f_sync
        .globl __next_fs_real_f_read, __next_fs_real_f_write, __next_fs_real_f_seek
        .globl __next_fs_real_f_fgetpos, __next_fs_real_f_fstat, __next_fs_real_f_getcwd
        .globl __next_fs_real_f_chdir, __next_fs_real_f_mkdir, __next_fs_real_f_rmdir
        .globl __next_fs_real_f_stat, __next_fs_real_f_unlink, __next_fs_real_f_rename
        .globl __next_fs_real_f_opendir, __next_fs_real_f_readdir
        .globl __next_fs_real_f_rewinddir, __next_fs_real_disk_info
        .globl __next_fs_real_m_execcmd
        .globl _nex_open, _nex_close, _nex_read, _nex_seek, _nex_position
        .globl _nex_fstat, _nex_stat, _nex_copy
        .globl _enter_critical_section, _leave_critical_section
        .area _CODE

        ;; A=drive, HL=path, B=mode.
__next_fs_f_open::
        push af
        push hl
        push bc
        ld a,b
        cp #1
        jr nz,.open_real
        call _nex_open
        ld a,d
        cp #255
        jr z,.open_real
        pop bc
        pop hl
        pop af
        ld a,e
        add a,#0xe0
        or a                    ; clear carry
        ret
.open_real:
        pop bc
        pop hl
        pop af
        jp __next_fs_real_f_open

__next_fs_f_close::
        cp #0xe0
        jp c,__next_fs_real_f_close
        sub #0xe0
        ld l,a
        ld h,#0
        call _nex_close
        jp .result

__next_fs_f_sync::
        cp #0xe0
        jp c,__next_fs_real_f_sync
        xor a
        ret

__next_fs_f_read::
        cp #0xe0
        jp c,__next_fs_real_f_read
        sub #0xe0
        ex de,hl
        ld l,a
        ld h,#0
        push bc
        call _nex_read
        ld b,d
        ld c,e
        jp .result

        ;; Embedded files only support absolute 16-bit seeks.
__next_fs_f_seek::
        cp #0xe0
        jp c,__next_fs_real_f_seek
        push af
        ld a,b
        or c
        or l
        jr nz,.seek_bad
        pop af
        sub #0xe0
        ld l,a
        ld h,#0
        call _nex_seek
        jp .result
.seek_bad:
        pop af
        ld a,#3
        scf
        ret

__next_fs_f_fgetpos::
        cp #0xe0
        jp c,__next_fs_real_f_fgetpos
        sub #0xe0
        ld l,a
        ld h,#0
        call _nex_position
        ld bc,#0
        xor a
        ret

__next_fs_f_fstat::
        cp #0xe0
        jp c,__next_fs_real_f_fstat
        sub #0xe0
        ex de,hl
        ld l,a
        ld h,#0
        call _nex_fstat
        jp .result

        ;; A=drive, HL=path, DE=status buffer.
__next_fs_f_stat::
        push af
        push hl
        push de
        call _nex_stat
        ld a,d
        cp #255
        jr z,.stat_real
        pop de
        pop hl
        pop af
        xor a
        ret
.stat_real:
        pop de
        pop hl
        pop af
        jp __next_fs_real_f_stat

.result:
        ld a,d
        cp #255
        jr z,.bad_handle
        xor a
        ret
.bad_handle:
        ld a,#13                ; native EBADF
        scf
        ret

__next_fs_f_write::
        jp __next_fs_real_f_write
__next_fs_f_getcwd::
        jp __next_fs_real_f_getcwd
__next_fs_f_chdir::
        jp __next_fs_real_f_chdir
__next_fs_f_mkdir::
        jp __next_fs_real_f_mkdir
__next_fs_f_rmdir::
        jp __next_fs_real_f_rmdir
__next_fs_f_unlink::
        jp __next_fs_real_f_unlink
__next_fs_f_rename::
        jp __next_fs_real_f_rename
__next_fs_f_opendir::
        jp __next_fs_real_f_opendir
__next_fs_f_readdir::
        jp __next_fs_real_f_readdir
__next_fs_f_rewinddir::
        jp __next_fs_real_f_rewinddir
        ;; NextZXOS addresses its SD card through the default drive, so the
        ;; desktop gets one fixed whole-disk record instead of probing devices.
__next_fs_disk_info::
        cp #0x80
        jr nz,.no_device
        ld (hl),a
        inc hl
        ld (hl),#1
        inc hl
        ld (hl),#0
        inc hl
        ld (hl),#3
        inc hl
        ld (hl),#0
        inc hl
        ld (hl),#0
        xor a
        ret
.no_device:
        ld a,#14
        scf
        ret
__next_fs_m_execcmd::
        jp __next_fs_real_m_execcmd
;; Source is the archive page mapped at A000-BFFF (MMU slot 5), destinations
;; are the fixed heap or a process bank. Slot 5 hides the top of the heap and
;; the +3DOS area while it is borrowed, so the copy goes through a small
;; staging buffer on a private stack in the kernel data (below A000), and the
;; mapping is restored before the bytes move on to their destination.
;; Interrupts stay disabled throughout (critical section).
        .equ NC_STAGE, 128
_nex_copy::
        push ix
        ld ix,#0
        add ix,sp
        ld (nc_src),hl
        ld (nc_dst),de
        call _enter_critical_section
        ld l,4(ix)
        ld h,5(ix)
        ld (nc_cnt),hl
        ld a,6(ix)
        ld (nc_page),a
        ld bc,#0x243b
        ld a,#0x55
        out (c),a
        inc b
        in a,(c)
        ld (nc_old),a
        ld (nc_sp),sp
        ld sp,#nc_stack_top
nc_loop:
        ld bc,(nc_cnt)
        ld a,b
        or c
        jr z,nc_done
        ld a,b
        or a
        jr nz,nc_cap
        ld a,c
        cp #NC_STAGE+1
        jr c,nc_have
nc_cap:
        ld a,#NC_STAGE
nc_have:
        ld (nc_n),a
        ld bc,#0x243b
        ld a,#0x55
        out (c),a
        inc b
        ld a,(nc_page)
        out (c),a
        ld a,(nc_n)
        ld c,a
        ld b,#0
        ld hl,(nc_src)
        ld de,#nc_stage
        ldir
        ld (nc_src),hl
        ld bc,#0x243b
        ld a,#0x55
        out (c),a
        inc b
        ld a,(nc_old)
        out (c),a
        ld a,(nc_n)
        ld c,a
        ld b,#0
        ld hl,#nc_stage
        ld de,(nc_dst)
        ldir
        ld (nc_dst),de
        ld hl,(nc_cnt)
        ld a,(nc_n)
        ld c,a
        ld b,#0
        or a
        sbc hl,bc
        ld (nc_cnt),hl
        jr nc_loop
nc_done:
        ld sp,(nc_sp)
        call _leave_critical_section
        pop ix
        pop hl
        inc sp
        inc sp
        inc sp
        jp (hl)

        .area _BSS
nc_src:  .ds 2
nc_dst:  .ds 2
nc_cnt:  .ds 2
nc_sp:   .ds 2
nc_page: .ds 1
nc_old:  .ds 1
nc_n:    .ds 1
nc_stage: .ds NC_STAGE
        .ds 32
nc_stack_top:
