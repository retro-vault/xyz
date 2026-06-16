        ;; cpm3_parse_path.s
        ;; Split from cpm3_fill_spaces.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module cpm3_parse_path
        .optsdcc -mz80 sdcccall(1)

        .globl  __cpm3_parse_path
        .globl  __cpm3_tmp_user
        .globl  __cpm3_get_current_user
        .globl  __cpm3_tmp_ptr
        .globl  __cpm3_zero_bytes

BDOS            .equ 5
DRV_GET         .equ 25
FCB_SIZE        .equ 36

        .area   _DATA
__cpm3_tmp_ptr2:
        .dw     0
        .area   _CODE
__cpm3_fill_spaces:
        ld      a,#' '
__cpm3_fill_spaces_loop:
        ld      (hl),a
        inc     hl
        djnz    __cpm3_fill_spaces_loop
        ret

        ;; HL = src, DE = dst, B = byte count.
__cpm3_upper_a:
        cp      #'a'
        jr      c,__cpm3_upper_a_done
        cp      #'z' + 1
        jr      nc,__cpm3_upper_a_done
        sub     #32
__cpm3_upper_a_done:
        ret

        ;; Validate A as a CP/M filename character. Carry set when accepted.
__cpm3_is_name_char:
        cp      #'0'
        jr      c,__cpm3_name_char_special
        cp      #'9' + 1
        scf
        jr      c,__cpm3_name_char_ret
        cp      #'A'
        jr      c,__cpm3_name_char_special
        cp      #'Z' + 1
        scf
        jr      c,__cpm3_name_char_ret
__cpm3_name_char_special:
        cp      #'_'
        jr      z,__cpm3_name_char_ok
        cp      #'$'
        jr      z,__cpm3_name_char_ok
        cp      #'#'
        jr      z,__cpm3_name_char_ok
        cp      #'@'
        jr      z,__cpm3_name_char_ok
        or      a
        ret
__cpm3_name_char_ok:
        scf
__cpm3_name_char_ret:
        ret

__cpm3_get_current_drive:
        push    ix
        push    iy
        ld      c,#DRV_GET
        ld      e,#0x00
        call    BDOS
        pop     iy
        pop     ix
        inc     a
        ret

        ;; HL = path, DE = destination FCB. Writes default drive/user plus
        ;; upper-cased 8.3 name. On success A = 0 and __cpm3_tmp_user holds
        ;; the selected user area.
__cpm3_parse_path::
        ld      a,h
        or      l
        jp      z,__cpm3_parse_fail
        ld      (__cpm3_tmp_ptr),hl
        ex      de,hl
        ld      (__cpm3_tmp_ptr2),hl
        ld      b,#FCB_SIZE
        call    __cpm3_zero_bytes
        ld      hl,(__cpm3_tmp_ptr2)
        inc     hl                      ; FCB name field at fcb+1
        ld      b,#8
        call    __cpm3_fill_spaces
        ld      b,#3
        call    __cpm3_fill_spaces      ; extension follows the name
        call    __cpm3_get_current_drive
        ld      hl,(__cpm3_tmp_ptr2)
        ld      (hl),a
        call    __cpm3_get_current_user
        ld      (__cpm3_tmp_user),a
        ld      hl,(__cpm3_tmp_ptr)
        ld      a,(hl)
        or      a
        jp      z,__cpm3_parse_fail
        call    __cpm3_upper_a
        ld      c,a
        inc     hl
        ld      a,(hl)
        cp      #':'
        jr      nz,__cpm3_parse_no_drive
        ld      a,c
        cp      #'A'
        jp      c,__cpm3_parse_fail
        cp      #'P' + 1
        jp      nc,__cpm3_parse_fail
        sub     #'A' - 1
        ld      de,(__cpm3_tmp_ptr2)
        ld      (de),a
        inc     hl
        jr      __cpm3_parse_name_start
__cpm3_parse_no_drive:
        dec     hl                      ; back to the first name char
__cpm3_parse_name_start:
        ld      de,(__cpm3_tmp_ptr2)
        inc     de
        ld      c,#0
__cpm3_parse_name_loop:
        ld      a,(hl)
        or      a
        jr      z,__cpm3_parse_name_done
        cp      #'.'
        jr      z,__cpm3_parse_ext
        cp      #'['
        jr      z,__cpm3_parse_user
        call    __cpm3_upper_a
        push    af
        call    __cpm3_is_name_char
        jp      nc,__cpm3_parse_fail_pop
        pop     af
        ld      a,c
        cp      #8
        jp      nc,__cpm3_parse_fail
        ld      a,(hl)
        call    __cpm3_upper_a
        ld      (de),a
        inc     de
        inc     hl
        inc     c
        jr      __cpm3_parse_name_loop
__cpm3_parse_name_done:
        ld      a,c
        or      a
        jp      z,__cpm3_parse_fail
        xor     a
        ret
__cpm3_parse_ext:
        ld      a,c
        or      a
        jp      z,__cpm3_parse_fail
        inc     hl
        ld      de,(__cpm3_tmp_ptr2)
        ld      a,e
        add     a,#9
        ld      e,a
        ld      a,d
        adc     a,#0
        ld      d,a
        ld      c,#0
__cpm3_parse_ext_loop:
        ld      a,(hl)
        or      a
        jr      z,__cpm3_parse_ext_done
        cp      #'['
        jr      z,__cpm3_parse_user
        call    __cpm3_upper_a
        push    af
        call    __cpm3_is_name_char
        jp      nc,__cpm3_parse_fail_pop
        pop     af
        ld      a,c
        cp      #3
        jp      nc,__cpm3_parse_fail
        ld      a,(hl)
        call    __cpm3_upper_a
        ld      (de),a
        inc     de
        inc     hl
        inc     c
        jr      __cpm3_parse_ext_loop
__cpm3_parse_ext_done:
        xor     a
        ret
__cpm3_parse_user:
        inc     hl
        ld      a,(hl)
        call    __cpm3_upper_a
        cp      #'G'
        jr      nz,__cpm3_parse_user_digit
        inc     hl
        ld      a,(hl)
        call    __cpm3_upper_a
__cpm3_parse_user_digit:
        cp      #'0'
        jp      c,__cpm3_parse_fail
        cp      #'9' + 1
        jp      nc,__cpm3_parse_fail
        sub     #'0'
        ld      b,a
        inc     hl
        ld      a,(hl)
        cp      #'0'
        jr      c,__cpm3_parse_user_tail
        cp      #'9' + 1
        jr      nc,__cpm3_parse_user_tail
        ld      a,b
        add     a,a
        ld      c,a
        add     a,a
        add     a,a
        add     a,c
        ld      b,a
        ld      a,(hl)
        sub     #'0'
        add     a,b
        ld      b,a
        inc     hl
__cpm3_parse_user_tail:
        ld      a,(hl)
        cp      #']'
        jp      nz,__cpm3_parse_fail
        ld      a,b
        cp      #16
        jp      nc,__cpm3_parse_fail
        ld      (__cpm3_tmp_user),a
        xor     a
        ret
__cpm3_parse_fail_pop:
        pop     af
__cpm3_parse_fail:
        ld      a,#1
        ret

        ;; IY = current entry. Clear the whole slot.
