        ;; sys_none_name_eq.s
        ;; Split from fileio.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module sys_none_name_eq
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_none_find_mount
        .globl  __sys_none_search_name
        .globl  __sys_none_slot_to_mount_ptr

MOUNT_COUNT     .equ 4

        .area   _CODE
__sys_none_name_eq:
        ld      a,(de)
        ld      c,a
        ld      a,(hl)
        cp      c
        ret     nz
        or      a
        ret     z
        inc     hl
        inc     de
        jr      __sys_none_name_eq

        ;; HL = path. Return HL = mount entry or 0.
__sys_none_find_mount::
        ld      (__sys_none_search_name),hl
        xor     a
        ld      b,#MOUNT_COUNT
__sys_none_find_mount_loop:
        push    af
        call    __sys_none_slot_to_mount_ptr
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ld      a,d
        or      e
        jr      z,__sys_none_find_mount_next
        ld      hl,(__sys_none_search_name)
        call    __sys_none_name_eq
        jr      z,__sys_none_find_mount_hit
__sys_none_find_mount_next:
        pop     af
        inc     a
        djnz    __sys_none_find_mount_loop
        ld      hl,#0x0000
        ret
__sys_none_find_mount_hit:
        pop     af
        call    __sys_none_slot_to_mount_ptr
        ret

        ;; Return A = free mount slot and HL = entry, or carry set on failure.
