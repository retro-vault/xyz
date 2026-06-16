        ;; sys_none_write.s
        ;; Split from fileio.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module sys_none_write
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_none_write
        .globl  __sys_none_find_open
        .globl  __sys_none_mount_terminate
        .globl  __sys_none_tmp_buf
        .globl  __sys_none_tmp_cap
        .globl  __sys_none_tmp_count
        .globl  __sys_none_tmp_len
        .globl  __sys_none_tmp_mount
        .globl  __sys_none_tmp_open
        .globl  __sys_none_tmp_ptr
        .globl  __sys_putchar

ACC_MASK        .equ 3
APPEND_FLAG     .equ 0x80
MOUNT_OFF_BUF   .equ 2
MOUNT_OFF_LEN   .equ 4
OPEN_OFF_FLAGS  .equ 2
OPEN_OFF_POS    .equ 3

        .area   _CODE
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
