        ;; cpm3_set_user_a.s
        ;; Split from cpm3_get_current_user.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module cpm3_set_user_a
        .optsdcc -mz80 sdcccall(1)

        .globl  __cpm3_set_user_a

BDOS            .equ 5
F_USERNUM       .equ 32

        .area   _CODE
__cpm3_set_user_a::
        push    ix
        push    iy
        push    bc
        push    de
        push    hl
        ld      e,a
        ld      c,#F_USERNUM
        call    BDOS
        pop     hl
        pop     de
        pop     bc
        pop     iy
        pop     ix
        ret

