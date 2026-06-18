        ;; rename.s
        ;; Split from fileio.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module rename
        .optsdcc -mz80 sdcccall(1)

        .globl  _rename
        .globl  __cpm3_copy_bytes
        .globl  __cpm3_get_current_user
        .globl  __cpm3_parse_path
        .globl  __cpm3_set_user_a
        .globl  __cpm3_tmp_fcb
        .globl  __cpm3_tmp_saved_user
        .globl  __cpm3_tmp_user

        .equ    BDOS,5
        .equ    BDOS_SUCCESS,0
        .equ    FCB_OFF_DRIVE,0
        .equ    FCB_SIZE,36
        .equ    F_RENAME,23

        .area   _DATA
__cpm3_tmp_fcb2:
        .ds     FCB_SIZE

__cpm3_tmp_user2:
        .db     0
        .area   _CODE
_rename::
        ld      a,h
        or      l
        jr      z,__cpm3_rename_fail
        ld      a,d
        or      e
        jr      z,__cpm3_rename_fail
        push    de
        ld      de,#__cpm3_tmp_fcb
        call    __cpm3_parse_path
        jr      nz,__cpm3_rename_fail_pop
        ld      a,(__cpm3_tmp_user)
        ld      (__cpm3_tmp_user2),a
        pop     hl
        push    hl
        ex      de,hl
        ld      de,#__cpm3_tmp_fcb2
        call    __cpm3_parse_path
        jr      nz,__cpm3_rename_fail_pop
        ld      a,(__cpm3_tmp_user2)
        ld      b,a
        ld      a,(__cpm3_tmp_user)
        cp      b
        jr      nz,__cpm3_rename_fail_pop
        ld      a,(__cpm3_tmp_fcb + FCB_OFF_DRIVE)
        ld      b,a
        ld      a,(__cpm3_tmp_fcb2 + FCB_OFF_DRIVE)
        cp      b
        jr      nz,__cpm3_rename_fail_pop
        ld      hl,#__cpm3_tmp_fcb2
        ld      de,#__cpm3_tmp_fcb + 16
        ld      b,#12
        call    __cpm3_copy_bytes
        call    __cpm3_get_current_user
        ld      (__cpm3_tmp_saved_user),a
        ld      a,(__cpm3_tmp_user)
        call    __cpm3_set_user_a
        ld      de,#__cpm3_tmp_fcb
        push    ix
        push    iy
        ld      c,#F_RENAME
        call    BDOS
        pop     iy
        pop     ix
        push    af
        ld      a,(__cpm3_tmp_saved_user)
        call    __cpm3_set_user_a
        pop     af
        pop     hl
        cp      #BDOS_SUCCESS
        jr      nz,__cpm3_rename_fail
        ld      de,#0x0000
        ret
__cpm3_rename_fail_pop:
        pop     hl
__cpm3_rename_fail:
        ld      de,#0xffff
        ret

