        ;; sys_none_find_open.s
        ;; Split from fileio.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module sys_none_find_open
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_none_find_open
        .globl  __sys_none_slot_to_open_ptr

FD_FILE_BASE    .equ 3
OPEN_COUNT      .equ 4

        .area   _CODE
__sys_none_find_open::
        ld      a,h
        or      a
        jr      nz,__sys_none_find_open_fail
        ld      a,l
        sub     #FD_FILE_BASE
        jr      c,__sys_none_find_open_fail
        cp      #OPEN_COUNT
        jr      nc,__sys_none_find_open_fail
        call    __sys_none_slot_to_open_ptr
        ld      a,(hl)
        inc     hl
        or      (hl)
        dec     hl
        ret     nz
__sys_none_find_open_fail:
        ld      hl,#0x0000
        ret

        ;; HL = mount entry. Write a trailing NUL when len < cap.
