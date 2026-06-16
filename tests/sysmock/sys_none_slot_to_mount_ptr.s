        ;; sys_none_slot_to_mount_ptr.s
        ;; Split from fileio.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module sys_none_slot_to_mount_ptr
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_none_search_name
        .globl  __sys_none_slot_to_mount_ptr
        .globl  __sys_none_mount_table

        .area   _DATA
__sys_none_search_name::
        .dw     0
        .area   _CODE
__sys_none_slot_to_mount_ptr::
        ld      l,a
        ld      h,#0x00
        add     hl,hl
        add     hl,hl
        add     hl,hl
        ld      de,#__sys_none_mount_table
        add     hl,de
        ret

        ;; A = open slot index -> HL = entry pointer.
