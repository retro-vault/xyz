        ;; sys_none_open.s
        ;; Split from fileio.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module sys_none_open
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_none_open
        .globl  __sys_none_find_mount
        .globl  __sys_none_mount_terminate
        .globl  __sys_none_search_name
        .globl  __sys_none_slot_to_mount_ptr
        .globl  __sys_none_slot_to_open_ptr
        .globl  __sys_none_tmp_mount

ACC_MASK        .equ 3
APPEND_FLAG     .equ 0x80
AUTO_FILE_CAP   .equ 256
FD_FILE_BASE    .equ 3
MOUNT_COUNT     .equ 4
MOUNT_OFF_LEN   .equ 4
OPEN_COUNT      .equ 4
OPEN_OFF_POS    .equ 3
O_CREAT_HI      .equ 0x01

        .area   _DATA
__sys_none_tmp_flags:
        .dw     0
__sys_none_auto_buffers:
        .ds     MOUNT_COUNT * AUTO_FILE_CAP

        .area   _CODE
__sys_none_find_free_mount:
        xor     a
        ld      b,#MOUNT_COUNT
__sys_none_find_free_mount_loop:
        push    af
        call    __sys_none_slot_to_mount_ptr
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ld      a,d
        or      e
        jr      z,__sys_none_find_free_mount_hit
        pop     af
        inc     a
        djnz    __sys_none_find_free_mount_loop
        scf
        ret
__sys_none_find_free_mount_hit:
        pop     af
        push    af
        call    __sys_none_slot_to_mount_ptr
        pop     af
        or      a
        ret

        ;; A = mount slot index -> HL = auto-created backing buffer.
__sys_none_slot_to_auto_buf:
        ld      hl,#__sys_none_auto_buffers
        or      a
        ret     z
        ld      b,a
__sys_none_slot_to_auto_buf_loop:
        ld      de,#AUTO_FILE_CAP
        add     hl,de
        djnz    __sys_none_slot_to_auto_buf_loop
        ret

        ;; HL = fd. Return HL = open entry or 0.
__sys_none_open:
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      a,h
        or      l
        jp      z,__sys_none_open_fail
        ld      (__sys_none_tmp_flags),de
        call    __sys_none_find_mount
        ld      a,h
        or      l
        jr      nz,__sys_none_open_have_mount
        ld      a,(__sys_none_tmp_flags + 1)
        and     #O_CREAT_HI
        jp      z,__sys_none_open_fail
        call    __sys_none_find_free_mount
        jp      c,__sys_none_open_fail
        ld      (__sys_none_tmp_mount),hl
        push    af
        ld      de,(__sys_none_search_name)
        ld      (hl),e
        inc     hl
        ld      (hl),d
        inc     hl
        pop     af
        push    hl
        call    __sys_none_slot_to_auto_buf
        ex      de,hl
        pop     hl
        ld      (hl),e
        inc     hl
        ld      (hl),d
        inc     hl
        xor     a
        ld      (hl),a
        inc     hl
        ld      (hl),a
        inc     hl
        ld      (hl),#0x00
        inc     hl
        ld      (hl),#0x01
        ld      hl,(__sys_none_tmp_mount)
        call    __sys_none_mount_terminate
        jr      __sys_none_open_store_from_tmp
__sys_none_open_have_mount:
        ld      (__sys_none_tmp_mount),hl
__sys_none_open_store_from_tmp:
        xor     a
        ld      b,#OPEN_COUNT
__sys_none_open_find_slot:
        push    af
        call    __sys_none_slot_to_open_ptr
        ld      a,(hl)
        inc     hl
        or      (hl)
        jr      z,__sys_none_open_store
        pop     af
        inc     a
        djnz    __sys_none_open_find_slot
        jp      __sys_none_open_fail
__sys_none_open_store:
        pop     af
        push    af
        call    __sys_none_slot_to_open_ptr
        ld      de,(__sys_none_tmp_mount)
        ld      (hl),e
        inc     hl
        ld      (hl),d
        inc     hl
        ld      a,(__sys_none_tmp_flags)
        and     #ACC_MASK
        ld      c,a
        ld      a,(__sys_none_tmp_flags + 1)
        and     #0x04
        jr      z,__sys_none_open_noappend
        ld      a,c
        or      #APPEND_FLAG
        ld      c,a
__sys_none_open_noappend:
        ld      a,c
        ld      (hl),a
        inc     hl
        xor     a
        ld      (hl),a
        inc     hl
        ld      (hl),a
        ld      a,(__sys_none_tmp_flags + 1)
        and     #0x02
        jr      z,__sys_none_open_notrunc
        ld      hl,(__sys_none_tmp_mount)
        ld      de,#MOUNT_OFF_LEN
        add     hl,de
        xor     a
        ld      (hl),a
        inc     hl
        ld      (hl),a
        ld      hl,(__sys_none_tmp_mount)
        call    __sys_none_mount_terminate
__sys_none_open_notrunc:
        ld      hl,(__sys_none_tmp_mount)
        ld      de,#MOUNT_OFF_LEN
        add     hl,de
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        pop     af
        push    af
        call    __sys_none_slot_to_open_ptr
        ld      b,h
        ld      c,l
        inc     hl
        inc     hl
        ld      a,(hl)
        and     #APPEND_FLAG
        jr      z,__sys_none_open_retfd
        ld      h,b
        ld      l,c
        ld      bc,#OPEN_OFF_POS
        add     hl,bc
        ld      (hl),e
        inc     hl
        ld      (hl),d
__sys_none_open_retfd:
        pop     af
        add     a,#FD_FILE_BASE
        ld      e,a
        ld      d,#0x00
        pop     ix
        ret
__sys_none_open_fail:
        ld      de,#0xffff
        pop     ix
        ret

