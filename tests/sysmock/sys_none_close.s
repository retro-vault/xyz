        ;; sys_none_close.s
        ;; Split from fileio.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module sys_none_close
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_none_close
        .globl  __sys_none_find_open

FD_FILE_BASE    .equ 3

        .area   _CODE
__sys_none_close:
        ld      a,h
        or      a
        jp      nz,__sys_none_close_fail
        ld      a,l
        cp      #FD_FILE_BASE
        jr      c,__sys_none_close_std
        call    __sys_none_find_open
        ld      a,h
        or      l
        jp      z,__sys_none_close_fail
        xor     a
        ld      (hl),a
        inc     hl
        ld      (hl),a
        inc     hl
        ld      (hl),a
        inc     hl
        ld      (hl),a
        inc     hl
        ld      (hl),a
__sys_none_close_std:
        ld      de,#0x0000
        ret
__sys_none_close_fail:
        ld      de,#0xffff
        ret

        ;; HL = path. Remove a mounted file when no open descriptor still
        ;; references the mount entry.
