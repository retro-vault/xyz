        ;; sys_none_slot_to_open_ptr.s
        ;; Split from fileio.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module sys_none_slot_to_open_ptr
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_none_slot_to_open_ptr
        .globl  __sys_none_open_table

        .area   _CODE
__sys_none_slot_to_open_ptr::
        ld      l,a
        ld      h,#0x00
        add     hl,hl
        push    hl
        add     hl,hl
        pop     de
        add     hl,de
        ld      de,#__sys_none_open_table
        add     hl,de
        ret

        ;; HL = path, DE = candidate name. Z when equal.
