        ;; sys_none_unlink.s
        ;; Split from fileio.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module sys_none_unlink
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_none_unlink
        .globl  __sys_none_find_mount
        .globl  __sys_none_slot_to_open_ptr
        .globl  __sys_none_tmp_mount

MOUNT_SIZE      .equ 8
OPEN_COUNT      .equ 4

        .area   _CODE
__sys_none_unlink:
        ld      a,h
        or      l
        jr      z,__sys_none_unlink_fail
        call    __sys_none_find_mount
        ld      a,h
        or      l
        jr      z,__sys_none_unlink_fail
        ld      (__sys_none_tmp_mount),hl
        xor     a
        ld      b,#OPEN_COUNT
__sys_none_unlink_scan:
        push    af
        call    __sys_none_slot_to_open_ptr
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ld      a,d
        or      e
        jr      z,__sys_none_unlink_next
        ld      hl,(__sys_none_tmp_mount)
        or      a
        sbc     hl,de
        jr      z,__sys_none_unlink_busy
__sys_none_unlink_next:
        pop     af
        inc     a
        djnz    __sys_none_unlink_scan
        ld      hl,(__sys_none_tmp_mount)
        xor     a
        ld      b,#MOUNT_SIZE
__sys_none_unlink_clear:
        ld      (hl),a
        inc     hl
        djnz    __sys_none_unlink_clear
        ld      de,#0x0000
        ret
__sys_none_unlink_busy:
        pop     af
__sys_none_unlink_fail:
        ld      de,#0xffff
        ret

        ;; HL = old path, DE = new path. Rename the mounted entry when the
        ;; source exists and the destination name is unused.
