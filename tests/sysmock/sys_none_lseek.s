        ;; sys_none_lseek.s
        ;; Split from fileio.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module sys_none_lseek
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_none_lseek
        .globl  __sys_none_find_open
        .globl  __sys_none_tmp_open

MOUNT_OFF_CAP   .equ 6
MOUNT_OFF_LEN   .equ 4
OPEN_OFF_POS    .equ 3
SEEK_CUR_V      .equ 1
SEEK_END_V      .equ 2
SEEK_SET_V      .equ 0

        .area   _CODE
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

