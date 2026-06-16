        ;; unlink.s
        ;; Split from fileio.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module unlink
        .optsdcc -mz80 sdcccall(1)

        .globl  _unlink
        .globl  __cpm3_get_current_user
        .globl  __cpm3_parse_path
        .globl  __cpm3_set_user_a
        .globl  __cpm3_tmp_fcb
        .globl  __cpm3_tmp_saved_user
        .globl  __cpm3_tmp_user

BDOS            .equ 5
BDOS_SUCCESS    .equ 0
F_DELETE        .equ 19

        .area   _CODE
_unlink::
        ld      a,h
        or      l
        jr      z,__cpm3_unlink_fail
        ld      de,#__cpm3_tmp_fcb
        call    __cpm3_parse_path
        jr      nz,__cpm3_unlink_fail
        call    __cpm3_get_current_user
        ld      (__cpm3_tmp_saved_user),a
        ld      a,(__cpm3_tmp_user)
        call    __cpm3_set_user_a
        ld      de,#__cpm3_tmp_fcb
        push    ix
        push    iy
        ld      c,#F_DELETE
        call    BDOS
        pop     iy
        pop     ix
        push    af
        ld      a,(__cpm3_tmp_saved_user)
        call    __cpm3_set_user_a
        pop     af
        cp      #BDOS_SUCCESS
        jr      nz,__cpm3_unlink_fail
        ld      de,#0x0000
        ret
__cpm3_unlink_fail:
        ld      de,#0xffff
        ret

