        ;; cpm3_find_open.s
        ;; Split from fileio.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).




        .module cpm3_find_open
        .optsdcc -mz80 sdcccall(1)

        .globl  __cpm3_find_open
        .globl  __cpm3_fd_slot_ptr

FD_FILE_BASE    .equ 3
OPEN_COUNT      .equ 16

        .area   _CODE
__cpm3_find_open::
        ld      a,h
        or      a
        jr      nz,__cpm3_find_open_fail
        ld      a,l
        sub     #FD_FILE_BASE
        jr      c,__cpm3_find_open_fail
        cp      #OPEN_COUNT
        jr      nc,__cpm3_find_open_fail
        call    __cpm3_fd_slot_ptr
        ld      a,(hl)
        or      a
        ret     nz
__cpm3_find_open_fail:
        ld      hl,#0x0000
        ret

        ;; Return A = free slot, HL = entry. Carry set on failure.
