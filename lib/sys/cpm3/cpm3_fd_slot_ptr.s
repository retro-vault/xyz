        ;; cpm3_fd_slot_ptr.s
        ;; Split from fileio.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).




        .module cpm3_fd_slot_ptr
        .optsdcc -mz80 sdcccall(1)

        .globl  __cpm3_fd_slot_ptr

FD_SIZE         .equ 181
OPEN_COUNT      .equ 16

        .area   _DATA
__cpm3_fd_table:
        .ds     OPEN_COUNT * FD_SIZE

        .area   _CODE
__cpm3_fd_slot_ptr::
        ld      hl,#__cpm3_fd_table
        or      a
        ret     z
        ld      b,a
__cpm3_fd_slot_ptr_loop:
        ld      de,#FD_SIZE
        add     hl,de
        djnz    __cpm3_fd_slot_ptr_loop
        ret

        ;; HL = fd. Return HL = entry or 0 on failure.
