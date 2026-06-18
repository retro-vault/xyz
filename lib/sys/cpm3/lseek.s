        ;; lseek.s
        ;; Split from fileio.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).




        .module lseek
        .optsdcc -mz80 sdcccall(1)

        .globl  _lseek
        .globl  __cpm3_tmp_rec2
        .globl  __cpm3_find_open
        .globl  __cpm3_get_current_user
        .globl  __cpm3_set_user_a
        .globl  __cpm3_sync_iy
        .globl  __cpm3_tmp_rec
        .globl  __cpm3_tmp_saved_user
        .globl  __cpm3_zero_tmprec

        .equ    FD_OFF_BUFVALID,4
        .equ    FD_OFF_DIRTY,3
        .equ    FD_OFF_FPOS0,5
        .equ    FD_OFF_FSIZE0,9
        .equ    FD_OFF_USER,2
        .equ    SEEK_CUR_V,1
        .equ    SEEK_END_V,2
        .equ    SEEK_SET_V,0

        .area   _CODE
_lseek::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __cpm3_find_open
        ld      a,h
        or      l
        jp      z,__cpm3_lseek_fail
        push    hl
        pop     iy
        call    __cpm3_get_current_user
        ld      (__cpm3_tmp_saved_user),a
        ld      a,FD_OFF_USER(iy)
        call    __cpm3_set_user_a
        call    __cpm3_sync_iy
        jp      nz,__cpm3_lseek_restore_fail
        ld      a,8(ix)
        or      9(ix)
        jp      nz,__cpm3_lseek_restore_fail
        ld      a,8(ix)
        cp      #SEEK_SET_V
        jr      z,__cpm3_lseek_from_start
        cp      #SEEK_CUR_V
        jr      z,__cpm3_lseek_from_cur
        cp      #SEEK_END_V
        jr      z,__cpm3_lseek_from_end
        jp      __cpm3_lseek_restore_fail
__cpm3_lseek_from_start:
        call    __cpm3_zero_tmprec
        jr      __cpm3_lseek_apply
__cpm3_lseek_from_cur:
        ld      a,FD_OFF_FPOS0(iy)
        ld      (__cpm3_tmp_rec),a
        ld      a,FD_OFF_FPOS0 + 1(iy)
        ld      (__cpm3_tmp_rec + 1),a
        ld      a,FD_OFF_FPOS0 + 2(iy)
        ld      (__cpm3_tmp_rec + 2),a
        ld      a,FD_OFF_FPOS0 + 3(iy)
        ld      (__cpm3_tmp_rec + 3),a
        jr      __cpm3_lseek_apply
__cpm3_lseek_from_end:
        ld      a,FD_OFF_FSIZE0(iy)
        ld      (__cpm3_tmp_rec),a
        ld      a,FD_OFF_FSIZE0 + 1(iy)
        ld      (__cpm3_tmp_rec + 1),a
        ld      a,FD_OFF_FSIZE0 + 2(iy)
        ld      (__cpm3_tmp_rec + 2),a
        ld      a,FD_OFF_FSIZE0 + 3(iy)
        ld      (__cpm3_tmp_rec + 3),a
__cpm3_lseek_apply:
        ld      a,7(ix)
        and     #0x80
        jr      z,__cpm3_lseek_add
        ld      a,4(ix)
        cpl
        ld      (__cpm3_tmp_rec2),a
        ld      a,5(ix)
        cpl
        ld      (__cpm3_tmp_rec2 + 1),a
        ld      a,6(ix)
        cpl
        ld      (__cpm3_tmp_rec2 + 2),a
        ld      a,7(ix)
        cpl
        ld      (__cpm3_tmp_rec2 + 3),a
        ld      hl,#__cpm3_tmp_rec2
        inc     (hl)
        jr      nz,__cpm3_lseek_sub
        inc     hl
        inc     (hl)
        jr      nz,__cpm3_lseek_sub
        inc     hl
        inc     (hl)
        jr      nz,__cpm3_lseek_sub
        inc     hl
        inc     (hl)
__cpm3_lseek_sub:
        ld      a,(__cpm3_tmp_rec2)
        ld      b,a
        ld      a,(__cpm3_tmp_rec)
        sub     b
        ld      (__cpm3_tmp_rec),a
        ld      a,(__cpm3_tmp_rec2 + 1)
        ld      b,a
        ld      a,(__cpm3_tmp_rec + 1)
        sbc     a,b
        ld      (__cpm3_tmp_rec + 1),a
        ld      a,(__cpm3_tmp_rec2 + 2)
        ld      b,a
        ld      a,(__cpm3_tmp_rec + 2)
        sbc     a,b
        ld      (__cpm3_tmp_rec + 2),a
        ld      a,(__cpm3_tmp_rec2 + 3)
        ld      b,a
        ld      a,(__cpm3_tmp_rec + 3)
        sbc     a,b
        ld      (__cpm3_tmp_rec + 3),a
        jp      c,__cpm3_lseek_restore_fail
        jr      __cpm3_lseek_check
__cpm3_lseek_add:
        ld      a,(__cpm3_tmp_rec)
        add     a,4(ix)
        ld      (__cpm3_tmp_rec),a
        ld      a,(__cpm3_tmp_rec + 1)
        adc     a,5(ix)
        ld      (__cpm3_tmp_rec + 1),a
        ld      a,(__cpm3_tmp_rec + 2)
        adc     a,6(ix)
        ld      (__cpm3_tmp_rec + 2),a
        ld      a,(__cpm3_tmp_rec + 3)
        adc     a,7(ix)
        ld      (__cpm3_tmp_rec + 3),a
        jp      c,__cpm3_lseek_restore_fail
__cpm3_lseek_check:
        ld      a,(__cpm3_tmp_rec + 3)
        cp      FD_OFF_FSIZE0 + 3(iy)
        jr      c,__cpm3_lseek_store
        jp      nz,__cpm3_lseek_restore_fail
        ld      a,(__cpm3_tmp_rec + 2)
        cp      FD_OFF_FSIZE0 + 2(iy)
        jr      c,__cpm3_lseek_store
        jp      nz,__cpm3_lseek_restore_fail
        ld      a,(__cpm3_tmp_rec + 1)
        cp      FD_OFF_FSIZE0 + 1(iy)
        jr      c,__cpm3_lseek_store
        jp      nz,__cpm3_lseek_restore_fail
        ld      a,(__cpm3_tmp_rec)
        cp      FD_OFF_FSIZE0(iy)
        jr      c,__cpm3_lseek_store
        jp      nz,__cpm3_lseek_restore_fail
__cpm3_lseek_store:
        ld      a,(__cpm3_tmp_rec)
        ld      FD_OFF_FPOS0(iy),a
        ld      a,(__cpm3_tmp_rec + 1)
        ld      FD_OFF_FPOS0 + 1(iy),a
        ld      a,(__cpm3_tmp_rec + 2)
        ld      FD_OFF_FPOS0 + 2(iy),a
        ld      a,(__cpm3_tmp_rec + 3)
        ld      FD_OFF_FPOS0 + 3(iy),a
        xor     a
        ld      FD_OFF_BUFVALID(iy),a
        ld      FD_OFF_DIRTY(iy),a
        ld      a,(__cpm3_tmp_saved_user)
        call    __cpm3_set_user_a
        ld      a,(__cpm3_tmp_rec)
        ld      e,a
        ld      a,(__cpm3_tmp_rec + 1)
        ld      d,a
        ld      a,(__cpm3_tmp_rec + 2)
        ld      l,a
        ld      a,(__cpm3_tmp_rec + 3)
        ld      h,a
        pop     ix
        ret
__cpm3_lseek_restore_fail:
        ld      a,(__cpm3_tmp_saved_user)
        call    __cpm3_set_user_a
__cpm3_lseek_fail:
        ld      de,#0xffff
        ld      hl,#0xffff
        pop     ix
        ret

