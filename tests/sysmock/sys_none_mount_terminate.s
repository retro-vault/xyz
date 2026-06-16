        ;; sys_none_mount_terminate.s
        ;; Split from fileio.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module sys_none_mount_terminate
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_none_mount_terminate

MOUNT_OFF_BUF   .equ 2

        .area   _CODE
__sys_none_mount_terminate::
        push    hl
        ld      de,#MOUNT_OFF_BUF
        add     hl,de
        ld      c,(hl)
        inc     hl
        ld      b,(hl)
        inc     hl
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        inc     hl
        ld      l,(hl)
        inc     hl
        ld      h,(hl)
        ld      a,d
        cp      h
        jr      c,__sys_none_mount_term_ok
        jr      nz,__sys_none_mount_term_done
        ld      a,e
        cp      l
        jr      nc,__sys_none_mount_term_done
__sys_none_mount_term_ok:
        ld      h,b
        ld      l,c
        add     hl,de
        xor     a
        ld      (hl),a
__sys_none_mount_term_done:
        pop     hl
        ret

