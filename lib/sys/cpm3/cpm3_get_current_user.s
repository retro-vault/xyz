        ;; cpm3_get_current_user.s
        ;; Split from fileio.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).




        .module cpm3_get_current_user
        .optsdcc -mz80 sdcccall(1)

        .globl  __cpm3_get_current_user

BDOS            .equ 5
F_USERNUM       .equ 32

        .area   _CODE
__cpm3_get_current_user::
        push    ix
        push    iy
        push    bc
        push    de
        push    hl
        ld      c,#F_USERNUM
        ld      e,#0xff
        call    BDOS
        pop     hl
        pop     de
        pop     bc
        pop     iy
        pop     ix
        ret

