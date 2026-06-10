        ;; fileio.s  (sys backend: none / bare metal)
        ;;
        ;; Simple buffer-backed file descriptors for tests. Named buffers can be
        ;; mounted into a tiny table, then opened through POSIX-style open/read/
        ;; write/lseek/close calls. This lets the libc stdio layer exercise real
        ;; file descriptors without needing a filesystem.
        ;;
        ;; Exported test helpers:
        ;;   __sys_file_reset(void)
        ;;   __sys_file_mount(const char *name, void *buf,
        ;;                    unsigned len, unsigned cap)
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module fileio
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_none_open
        .globl  __sys_none_close
        .globl  __sys_none_lseek
        .globl  __sys_none_read
        .globl  __sys_none_write
        .globl  __sys_none_unlink
        .globl  __sys_none_rename
        .globl  __sys_file_reset
        .globl  ___sys_file_reset
        .globl  __sys_file_mount
        .globl  ___sys_file_mount

        .globl  __sys_getchar
        .globl  __sys_putchar

FD_FILE_BASE    .equ 3
MOUNT_COUNT     .equ 4
OPEN_COUNT      .equ 4
AUTO_FILE_CAP   .equ 256

MOUNT_SIZE      .equ 8
MOUNT_OFF_NAME  .equ 0
MOUNT_OFF_BUF   .equ 2
MOUNT_OFF_LEN   .equ 4
MOUNT_OFF_CAP   .equ 6

OPEN_SIZE       .equ 5
OPEN_OFF_MOUNT  .equ 0
OPEN_OFF_FLAGS  .equ 2
OPEN_OFF_POS    .equ 3

ACC_RDONLY      .equ 0
ACC_WRONLY      .equ 1
ACC_RDWR        .equ 2
ACC_MASK        .equ 3
APPEND_FLAG     .equ 0x80
O_CREAT_HI      .equ 0x01

SEEK_SET_V      .equ 0
SEEK_CUR_V      .equ 1
SEEK_END_V      .equ 2

        .area   _DATA
__sys_none_mount_table:
        .ds     MOUNT_COUNT * MOUNT_SIZE
__sys_none_open_table:
        .ds     OPEN_COUNT * OPEN_SIZE

__sys_none_search_name:
        .dw     0
__sys_none_tmp_flags:
        .dw     0
__sys_none_tmp_buf:
        .dw     0
__sys_none_tmp_len:
        .dw     0
__sys_none_tmp_cap:
        .dw     0
__sys_none_tmp_open:
        .dw     0
__sys_none_tmp_mount:
        .dw     0
__sys_none_tmp_count:
        .dw     0
__sys_none_tmp_ptr:
        .dw     0
__sys_none_auto_buffers:
        .ds     MOUNT_COUNT * AUTO_FILE_CAP

        .area   _CODE

        ;; A = mount slot index -> HL = entry pointer.
__sys_none_slot_to_mount_ptr:
        ld      l,a
        ld      h,#0x00
        add     hl,hl
        add     hl,hl
        add     hl,hl
        ld      de,#__sys_none_mount_table
        add     hl,de
        ret

        ;; A = open slot index -> HL = entry pointer.
__sys_none_slot_to_open_ptr:
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
__sys_none_find_mount:
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
__sys_none_find_open:
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
__sys_none_mount_terminate:
        push    hl
        ld      de,#MOUNT_OFF_BUF
        add     hl,de
        ld      c,(hl)
        inc     hl
        ld      b,(hl)
        inc     hl
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        inc     hl
        ld      l,(hl)
        inc     hl
        ld      h,(hl)
        ld      a,d
        cp      h
        jr      c,__sys_none_mount_term_ok
        jr      nz,__sys_none_mount_term_done
        ld      a,e
        cp      l
        jr      nc,__sys_none_mount_term_done
__sys_none_mount_term_ok:
        ld      h,b
        ld      l,c
        add     hl,de
        xor     a
        ld      (hl),a
__sys_none_mount_term_done:
        pop     hl
        ret

__sys_file_reset:
___sys_file_reset::
        xor     a
        ld      hl,#__sys_none_mount_table
        ld      b,#(MOUNT_COUNT * MOUNT_SIZE)
__sys_none_reset_mount_loop:
        ld      (hl),a
        inc     hl
        djnz    __sys_none_reset_mount_loop
        ld      hl,#__sys_none_open_table
        ld      b,#(OPEN_COUNT * OPEN_SIZE)
__sys_none_reset_open_loop:
        ld      (hl),a
        inc     hl
        djnz    __sys_none_reset_open_loop
        ex      de,hl
        ret

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
__sys_none_rename:
        ld      a,h
        or      l
        jr      z,__sys_none_rename_fail
        ld      a,d
        or      e
        jr      z,__sys_none_rename_fail
        ld      (__sys_none_tmp_ptr),de
        ld      (__sys_none_search_name),hl
        call    __sys_none_find_mount
        ld      a,h
        or      l
        jr      z,__sys_none_rename_fail
        ld      (__sys_none_tmp_mount),hl
        ld      hl,(__sys_none_tmp_ptr)
        call    __sys_none_find_mount
        ld      a,h
        or      l
        jr      z,__sys_none_rename_new_free
        jr      __sys_none_rename_fail
__sys_none_rename_new_free:
        ld      hl,(__sys_none_tmp_mount)
        ld      de,(__sys_none_tmp_ptr)
        ld      (hl),e
        inc     hl
        ld      (hl),d
        ld      de,#0x0000
        ret
__sys_none_rename_fail:
        ld      de,#0xffff
        ret

__sys_none_lseek:
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __sys_none_find_open
        ld      a,h
        or      l
        jp      z,__sys_none_lseek_fail
        ld      (__sys_none_tmp_open),hl
        ld      e,8(ix)
        ld      d,9(ix)
        ld      a,d
        or      a
        jp      nz,__sys_none_lseek_fail
        ld      a,e
        cp      #SEEK_SET_V
        jr      z,__sys_none_lseek_base_set
        cp      #SEEK_CUR_V
        jr      z,__sys_none_lseek_base_cur
        cp      #SEEK_END_V
        jr      z,__sys_none_lseek_base_end
        jp      __sys_none_lseek_fail
__sys_none_lseek_base_set:
        ld      b,#0x00
        ld      c,#0x00
        jr      __sys_none_lseek_apply
__sys_none_lseek_base_cur:
        ld      hl,(__sys_none_tmp_open)
        ld      de,#OPEN_OFF_POS
        add     hl,de
        ld      c,(hl)
        inc     hl
        ld      b,(hl)
        jr      __sys_none_lseek_apply
__sys_none_lseek_base_end:
        ld      hl,(__sys_none_tmp_open)
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ex      de,hl
        ld      de,#MOUNT_OFF_LEN
        add     hl,de
        ld      c,(hl)
        inc     hl
        ld      b,(hl)
__sys_none_lseek_apply:
        ld      a,6(ix)
        or      a
        jr      nz,__sys_none_lseek_neg_hi
        ld      a,7(ix)
        or      a
        jr      z,__sys_none_lseek_pos
__sys_none_lseek_neg_hi:
        ld      a,6(ix)
        cp      #0xff
        jp      nz,__sys_none_lseek_fail
        ld      a,7(ix)
        cp      #0xff
        jp      nz,__sys_none_lseek_fail
        ld      e,4(ix)
        ld      d,5(ix)
        ld      a,e
        cpl
        ld      e,a
        ld      a,d
        cpl
        ld      d,a
        inc     de
        ld      a,c
        sub     e
        ld      c,a
        ld      a,b
        sbc     a,d
        ld      b,a
        jp      c,__sys_none_lseek_fail
        jr      __sys_none_lseek_check_cap
__sys_none_lseek_pos:
        ld      a,c
        add     a,4(ix)
        ld      c,a
        ld      a,b
        adc     a,5(ix)
        ld      b,a
        jp      c,__sys_none_lseek_fail
__sys_none_lseek_check_cap:
        ld      hl,(__sys_none_tmp_open)
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ex      de,hl
        ld      de,#MOUNT_OFF_CAP
        add     hl,de
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ld      a,b
        cp      d
        jr      c,__sys_none_lseek_store
        jp      nz,__sys_none_lseek_fail
        ld      a,c
        cp      e
        jr      c,__sys_none_lseek_store
        jp      nz,__sys_none_lseek_fail
__sys_none_lseek_store:
        ld      hl,(__sys_none_tmp_open)
        ld      de,#OPEN_OFF_POS
        add     hl,de
        ld      (hl),c
        inc     hl
        ld      (hl),b
        ld      e,c
        ld      d,b
        ld      hl,#0x0000
        pop     ix
        ret
__sys_none_lseek_fail:
        ld      de,#0xffff
        ld      hl,#0xffff
        pop     ix
        ret

__sys_none_read:
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      a,h
        or      a
        jp      nz,__sys_none_read_badfd
        ld      a,l
        or      a
        jr      nz,__sys_none_read_file
        ld      c,4(ix)
        ld      b,5(ix)
        ld      h,d
        ld      l,e
        push    bc
__sys_none_read_console_loop:
        ld      a,b
        or      c
        jr      z,__sys_none_read_console_done
        push    hl
        call    __sys_getchar
        ld      a,d
        cp      #0xff
        jr      nz,__sys_none_read_console_store
        ld      a,e
        cp      #0xff
        jr      z,__sys_none_read_console_done_pop
__sys_none_read_console_store:
        pop     hl
        ld      (hl),e
        inc     hl
        dec     bc
        jr      __sys_none_read_console_loop
__sys_none_read_console_done_pop:
        pop     hl
__sys_none_read_console_done:
        pop     hl
        or      a
        sbc     hl,bc
        ex      de,hl
        pop     ix
        ret
__sys_none_read_file:
        ld      (__sys_none_tmp_ptr),de
        call    __sys_none_find_open
        ld      a,h
        or      l
        jp      z,__sys_none_read_badfd
        ld      (__sys_none_tmp_open),hl
        ld      de,#OPEN_OFF_FLAGS
        add     hl,de
        ld      a,(hl)
        and     #ACC_MASK
        cp      #ACC_WRONLY
        jp      z,__sys_none_read_badfd
        ld      hl,(__sys_none_tmp_open)
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ex      de,hl
        ld      (__sys_none_tmp_mount),hl
        ld      de,#MOUNT_OFF_BUF
        add     hl,de
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ld      (__sys_none_tmp_buf),de
        inc     hl
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ld      (__sys_none_tmp_len),de
        ld      hl,(__sys_none_tmp_open)
        ld      de,#OPEN_OFF_POS
        add     hl,de
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ld      c,4(ix)
        ld      b,5(ix)
        ld      (__sys_none_tmp_count),bc
        ld      hl,(__sys_none_tmp_ptr)
__sys_none_read_file_loop:
        ld      a,b
        or      c
        jr      z,__sys_none_read_file_done
        push    hl
        ld      hl,(__sys_none_tmp_len)
        ld      a,d
        cp      h
        jr      c,__sys_none_read_file_have_pop
        jr      nz,__sys_none_read_file_done_pop
        ld      a,e
        cp      l
        jr      nc,__sys_none_read_file_done_pop
__sys_none_read_file_have_pop:
        pop     hl
__sys_none_read_file_have:
        push    bc
        push    de
        push    hl
        ld      hl,(__sys_none_tmp_buf)
        add     hl,de
        ld      a,(hl)
        pop     hl
        ld      (hl),a
        inc     hl
        pop     de
        inc     de
        pop     bc
        dec     bc
        jr      __sys_none_read_file_loop
__sys_none_read_file_done_pop:
        pop     hl
__sys_none_read_file_done:
        push    bc
        ld      hl,(__sys_none_tmp_open)
        ld      bc,#OPEN_OFF_POS
        add     hl,bc
        ld      (hl),e
        inc     hl
        ld      (hl),d
        pop     bc
        ld      hl,(__sys_none_tmp_count)
        or      a
        sbc     hl,bc
        ex      de,hl
        pop     ix
        ret
__sys_none_read_badfd:
        ld      de,#0xffff
        pop     ix
        ret

__sys_none_write:
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      a,h
        or      a
        jp      nz,__sys_none_write_badfd
        ld      a,l
        cp      #1
        jr      z,__sys_none_write_console
        cp      #2
        jr      z,__sys_none_write_console
        jp      __sys_none_write_file
__sys_none_write_console:
        ld      c,4(ix)
        ld      b,5(ix)
        ld      h,d
        ld      l,e
__sys_none_write_console_loop:
        ld      a,b
        or      c
        jr      z,__sys_none_write_console_done
        ld      a,(hl)
        push    hl
        push    bc
        ld      l,a
        call    __sys_putchar
        pop     bc
        pop     hl
        inc     hl
        dec     bc
        jr      __sys_none_write_console_loop
__sys_none_write_console_done:
        ld      e,4(ix)
        ld      d,5(ix)
        pop     ix
        ret
__sys_none_write_file:
        ld      (__sys_none_tmp_ptr),de
        call    __sys_none_find_open
        ld      a,h
        or      l
        jp      z,__sys_none_write_badfd
        ld      (__sys_none_tmp_open),hl
        ld      de,#OPEN_OFF_FLAGS
        add     hl,de
        ld      a,(hl)
        and     #ACC_MASK
        jp      z,__sys_none_write_badfd
        ld      hl,(__sys_none_tmp_open)
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ex      de,hl
        ld      (__sys_none_tmp_mount),hl
        ld      de,#MOUNT_OFF_BUF
        add     hl,de
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ld      (__sys_none_tmp_buf),de
        inc     hl
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ld      (__sys_none_tmp_len),de
        inc     hl
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ld      (__sys_none_tmp_cap),de
        ld      hl,(__sys_none_tmp_open)
        ld      de,#OPEN_OFF_FLAGS
        add     hl,de
        ld      a,(hl)
        and     #APPEND_FLAG
        jr      z,__sys_none_write_use_pos
        ld      de,(__sys_none_tmp_len)
        jr      __sys_none_write_pos_ready
__sys_none_write_use_pos:
        ld      hl,(__sys_none_tmp_open)
        ld      de,#OPEN_OFF_POS
        add     hl,de
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
__sys_none_write_pos_ready:
        ld      hl,(__sys_none_tmp_ptr)
        ld      c,4(ix)
        ld      b,5(ix)
        ld      (__sys_none_tmp_count),bc
__sys_none_write_file_loop:
        ld      a,b
        or      c
        jr      z,__sys_none_write_file_done
        push    hl
        ld      hl,(__sys_none_tmp_cap)
        ld      a,d
        cp      h
        jr      c,__sys_none_write_file_room_pop
        jr      nz,__sys_none_write_file_done_pop
        ld      a,e
        cp      l
        jr      nc,__sys_none_write_file_done_pop
__sys_none_write_file_room_pop:
        pop     hl
__sys_none_write_file_room:
        push    bc
        push    de
        ld      a,(hl)
        inc     hl
        push    hl
        ld      hl,(__sys_none_tmp_buf)
        add     hl,de
        ld      (hl),a
        pop     hl
        pop     de
        inc     de
        pop     bc
        dec     bc
        jr      __sys_none_write_file_loop
__sys_none_write_file_done_pop:
        pop     hl
__sys_none_write_file_done:
        push    bc
        ld      hl,(__sys_none_tmp_open)
        ld      bc,#OPEN_OFF_POS
        add     hl,bc
        ld      (hl),e
        inc     hl
        ld      (hl),d
        pop     bc
        ld      hl,(__sys_none_tmp_len)
        ld      a,d
        cp      h
        jr      c,__sys_none_write_len_ok
        jr      nz,__sys_none_write_len_update
        ld      a,e
        cp      l
        jr      c,__sys_none_write_len_ok
        jr      z,__sys_none_write_len_ok
__sys_none_write_len_update:
        push    bc
        ld      hl,(__sys_none_tmp_mount)
        ld      bc,#MOUNT_OFF_LEN
        add     hl,bc
        ld      (hl),e
        inc     hl
        ld      (hl),d
        ld      hl,(__sys_none_tmp_mount)
        call    __sys_none_mount_terminate
        pop     bc
__sys_none_write_len_ok:
        ld      hl,(__sys_none_tmp_count)
        or      a
        sbc     hl,bc
        ex      de,hl
        pop     ix
        ret
__sys_none_write_badfd:
        ld      de,#0xffff
        pop     ix
        ret
