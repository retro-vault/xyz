        ;; cpm3_close_file.s  (sys backend: CP/M 3)
        ;;
        ;; FCB-backed close() path for file descriptors >= 3. Reached only
        ;; through __cpm3_close_file_vec, installed by _open.
        ;; Entry contract: HL = fd. Returns DE = 0 or 0xFFFF.

        .module cpm3_close_file
        .optsdcc -mz80 sdcccall(1)

        .globl  __cpm3_close_file
        .globl  __cpm3_clear_entry_iy
        .globl  __cpm3_entry_fcb_ptr_iy
        .globl  __cpm3_find_open
        .globl  __cpm3_get_current_user
        .globl  __cpm3_set_user_a
        .globl  __cpm3_sync_iy
        .globl  __cpm3_tmp_saved_user

BDOS            .equ 5
BDOS_SUCCESS    .equ 0
FD_OFF_USER     .equ 2
F_CLOSE         .equ 16

        .area   _CODE
__cpm3_close_file::
        call    __cpm3_find_open
        ld      a,h
        or      l
        jr      z,__cpm3_cf_fail
        push    hl
        pop     iy
        call    __cpm3_get_current_user
        ld      (__cpm3_tmp_saved_user),a
        ld      a,FD_OFF_USER(iy)
        call    __cpm3_set_user_a
        call    __cpm3_sync_iy
        jr      nz,__cpm3_cf_restore_fail
        call    __cpm3_entry_fcb_ptr_iy
        ex      de,hl
        push    ix
        push    iy
        ld      c,#F_CLOSE
        call    BDOS
        pop     iy
        pop     ix
        push    af
        ld      a,(__cpm3_tmp_saved_user)
        call    __cpm3_set_user_a
        pop     af
        cp      #BDOS_SUCCESS
        jr      nz,__cpm3_cf_fail
        call    __cpm3_clear_entry_iy
        ld      de,#0x0000
        ret
__cpm3_cf_restore_fail:
        ld      a,(__cpm3_tmp_saved_user)
        call    __cpm3_set_user_a
__cpm3_cf_fail:
        ld      de,#0xffff
        ret
