        ;; sys_none_read.s
        ;; Split from fileio.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module sys_none_read
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_none_read
        .globl  __sys_getchar
        .globl  __sys_none_find_open
        .globl  __sys_none_tmp_buf
        .globl  __sys_none_tmp_count
        .globl  __sys_none_tmp_len
        .globl  __sys_none_tmp_mount
        .globl  __sys_none_tmp_open
        .globl  __sys_none_tmp_ptr

ACC_MASK        .equ 3
ACC_WRONLY      .equ 1
MOUNT_OFF_BUF   .equ 2
OPEN_OFF_FLAGS  .equ 2
OPEN_OFF_POS    .equ 3

        .area   _CODE
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

