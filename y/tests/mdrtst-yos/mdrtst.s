        ;; mdrtst.s
        ;;
        ;; Tiny standalone microdrive save/load self-test app.
        ;; Keeps data buffers in BSS so the XL image itself stays small.

        .module mdrtst

        .globl  _entry
        .globl  _main
        .globl  _query_service
        .globl  ___sdcc_call_hl
        .globl  ___sdcc_call_iy

        .equ    YOS_PUTS,          12
        .equ    YOS_MALLOC,        24
        .equ    YOS_FREE,          26
        .equ    YOS_MDR_DETECT,    30
        .equ    YOS_MDR_LOAD,      34
        .equ    YOS_MDR_SAVE,      36

        .equ    LEN_123,           123
        .equ    LEN_512,           512
        .equ    LEN_777,           777

        .area   _CODE

_main::
        ld      hl,#str_yos
        call    _query_service
        ld      a,d
        or      e
        ret     z
        ld      (yos_ptr),de

        call    detect_drives
        ld      a,l
        or      a
        jr      nz,.have_drive
        ld      hl,#msg_nomdr
        call    call_puts
        ret

.have_drive:
        ld      hl,#msg_start
        call    call_puts

        ld      hl,#LEN_777
        call    call_malloc
        ld      a,d
        or      e
        jr      nz,.have_buf
        ld      hl,#msg_oom
        call    call_puts
        ret

.have_buf:
        ld      (buf_ptr),de
        ex      de,hl
        call    fill_pattern

        ld      hl,#name_123
        ld      de,(buf_ptr)
        ld      bc,#LEN_123
        call    call_save
        ld      a,l
        or      a
        jr      z,.save_512
        ld      hl,#msg_s123
        call    call_puts
        ret

.save_512:
        ld      hl,#name_512
        ld      de,(buf_ptr)
        ld      bc,#LEN_512
        call    call_save
        ld      a,l
        or      a
        jr      z,.save_777
        ld      hl,#msg_s512
        call    call_puts
        ret

.save_777:
        ld      hl,#name_777
        ld      de,(buf_ptr)
        ld      bc,#LEN_777
        call    call_save
        ld      a,l
        or      a
        jr      z,.load_123
        ld      hl,#msg_s777
        call    call_puts
        ret

.load_123:
        ld      hl,#name_123
        ld      de,(buf_ptr)
        call    call_load
        ld      a,l
        or      a
        jr      z,.cmp_123
        ld      hl,#msg_l123
        call    call_puts
        ret

.cmp_123:
        ld      hl,(buf_ptr)
        ld      bc,#LEN_123
        call    compare_pattern
        jr      nc,.load_512
        ld      hl,#msg_c123
        call    call_puts
        ret

.load_512:
        ld      hl,#name_512
        ld      de,(buf_ptr)
        call    call_load
        ld      a,l
        or      a
        jr      z,.cmp_512
        ld      hl,#msg_l512
        call    call_puts
        ret

.cmp_512:
        ld      hl,(buf_ptr)
        ld      bc,#LEN_512
        call    compare_pattern
        jr      nc,.load_777
        ld      hl,#msg_c512
        call    call_puts
        ret

.load_777:
        ld      hl,#name_777
        ld      de,(buf_ptr)
        call    call_load
        ld      a,l
        or      a
        jr      z,.cmp_777
        ld      hl,#msg_l777
        call    call_puts
        ret

.cmp_777:
        ld      hl,(buf_ptr)
        ld      bc,#LEN_777
        call    compare_pattern
        jr      nc,.pass
        ld      hl,#msg_c777
        call    call_puts
        ret

.pass:
        ld      hl,(buf_ptr)
        call    call_free
        ld      hl,#msg_pass
        call    call_puts
        ret

call_puts:
        push    hl
        ld      de,(yos_ptr)
        ld      hl,#YOS_PUTS
        add     hl,de
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        pop     hl
        push    de
        pop     iy
        call    ___sdcc_call_iy
        ret

detect_drives:
        ld      de,(yos_ptr)
        ld      hl,#YOS_MDR_DETECT
        add     hl,de
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ex      de,hl
        call    ___sdcc_call_hl
        ret

call_malloc:
        ld      de,(yos_ptr)
        ld      hl,#YOS_MALLOC
        add     hl,de
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        push    de
        pop     iy
        jp      (iy)

call_free:
        push    hl
        ld      de,(yos_ptr)
        ld      hl,#YOS_FREE
        add     hl,de
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        pop     hl
        push    de
        pop     iy
        jp      (iy)

call_save:
        ld      (tmp_name),hl
        ld      (tmp_ptr),de
        ld      (tmp_len),bc
        ld      de,(yos_ptr)
        ld      hl,#YOS_MDR_SAVE
        add     hl,de
        ld      c,(hl)
        inc     hl
        ld      b,(hl)
        ld      hl,(tmp_len)
        push    hl
        ld      hl,(tmp_ptr)
        push    hl
        ld      de,(tmp_name)
        ld      a,#1
        ld      l,c
        ld      h,b
        call    ___sdcc_call_hl
        ret

call_load:
        ld      (tmp_name),hl
        ld      (tmp_ptr),de
        ld      de,(yos_ptr)
        ld      hl,#YOS_MDR_LOAD
        add     hl,de
        ld      c,(hl)
        inc     hl
        ld      b,(hl)
        ld      hl,(tmp_ptr)
        push    hl
        ld      de,(tmp_name)
        ld      a,#1
        ld      l,c
        ld      h,b
        call    ___sdcc_call_hl
        ret

fill_pattern:
        ld      de,#LEN_777
        ld      c,#1
.fp_loop:
        ld      a,c
        ld      (hl),a
        inc     hl
        inc     c
        ld      a,c
        cp      #12
        jr      nz,.fp_next
        ld      c,#1
.fp_next:
        dec     de
        ld      a,d
        or      e
        jr      nz,.fp_loop
        ret

compare_pattern:
        ld      d,#1
.cmp_loop:
        ld      a,(hl)
        cp      d
        jr      nz,.cmp_fail
        inc     hl
        inc     d
        ld      a,d
        cp      #12
        jr      nz,.cmp_next
        ld      d,#1
.cmp_next:
        dec     bc
        ld      a,b
        or      c
        jr      nz,.cmp_loop
        or      a
        ret
.cmp_fail:
        scf
        ret

str_yos:
        .ascii  "yos"
        .db     0

name_123:
        .ascii  "t123"
        .db     0

name_512:
        .ascii  "t512"
        .db     0

name_777:
        .ascii  "t777"
        .db     0

msg_start:
        .ascii  "MDR TEST"
        .db     0

msg_nomdr:
        .ascii  "NO MDR"
        .db     0

msg_oom:
        .ascii  "NO MEM"
        .db     0

msg_s123:
        .ascii  "SAVE123"
        .db     0

msg_s512:
        .ascii  "SAVE512"
        .db     0

msg_s777:
        .ascii  "SAVE777"
        .db     0

msg_l123:
        .ascii  "LOAD123"
        .db     0

msg_l512:
        .ascii  "LOAD512"
        .db     0

msg_l777:
        .ascii  "LOAD777"
        .db     0

msg_c123:
        .ascii  "CMP123"
        .db     0

msg_c512:
        .ascii  "CMP512"
        .db     0

msg_c777:
        .ascii  "CMP777"
        .db     0

msg_pass:
        .ascii  "PASS"
        .db     0

        .area   _BSS

yos_ptr:
        .ds     2
tmp_name:
        .ds     2
tmp_ptr:
        .ds     2
tmp_len:
        .ds     2
buf_ptr:
        .ds     2
