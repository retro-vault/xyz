        ;; sys_file_mount.s
        ;; Split from fileio.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module sys_file_mount
        .optsdcc -mz80 sdcccall(1)

        .globl  ___sys_file_mount
        .globl  __sys_file_mount
        .globl  __sys_none_mount_terminate
        .globl  __sys_none_search_name
        .globl  __sys_none_slot_to_mount_ptr
        .globl  __sys_none_tmp_buf
        .globl  __sys_none_tmp_cap
        .globl  __sys_none_tmp_len

MOUNT_COUNT     .equ 4

        .area   _CODE
__sys_file_mount:
___sys_file_mount::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      a,h
        or      l
        jr      z,__sys_none_mount_fail
        ld      a,d
        or      e
        jr      z,__sys_none_mount_fail
        ld      c,6(ix)
        ld      b,7(ix)
        ld      a,b
        or      c
        jr      z,__sys_none_mount_fail
        ld      (__sys_none_search_name),hl
        ld      (__sys_none_tmp_buf),de
        ld      e,4(ix)
        ld      d,5(ix)
        ld      a,b
        cp      d
        jr      c,__sys_none_mount_clamp
        jr      nz,__sys_none_mount_len_ok
        ld      a,c
        cp      e
        jr      c,__sys_none_mount_clamp
        jr      z,__sys_none_mount_len_ok
__sys_none_mount_clamp:
        ld      d,b
        ld      e,c
__sys_none_mount_len_ok:
        ld      (__sys_none_tmp_len),de
        ld      (__sys_none_tmp_cap),bc
        xor     a
        ld      b,#MOUNT_COUNT
__sys_none_mount_find_slot:
        push    af
        call    __sys_none_slot_to_mount_ptr
        ld      a,(hl)
        inc     hl
        or      (hl)
        jr      z,__sys_none_mount_store
        pop     af
        inc     a
        djnz    __sys_none_mount_find_slot
        jr      __sys_none_mount_fail
__sys_none_mount_store:
        pop     af
        push    af
        call    __sys_none_slot_to_mount_ptr
        ld      de,(__sys_none_search_name)
        ld      (hl),e
        inc     hl
        ld      (hl),d
        inc     hl
        ld      de,(__sys_none_tmp_buf)
        ld      (hl),e
        inc     hl
        ld      (hl),d
        inc     hl
        ld      de,(__sys_none_tmp_len)
        ld      (hl),e
        inc     hl
        ld      (hl),d
        inc     hl
        ld      de,(__sys_none_tmp_cap)
        ld      (hl),e
        inc     hl
        ld      (hl),d
        pop     af
        call    __sys_none_slot_to_mount_ptr
        call    __sys_none_mount_terminate
        ld      e,a
        ld      d,#0x00
        pop     ix
        ret
__sys_none_mount_fail:
        ld      de,#0xffff
        pop     ix
        ret

