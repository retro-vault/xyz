        ;; cpm3_shift7_from_ptr.s
        ;; Split from cpm3_find_open.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module cpm3_shift7_from_ptr
        .optsdcc -mz80 sdcccall(1)

        .globl  __cpm3_shift7_from_ptr
        .globl  __cpm3_copy_bytes
        .globl  __cpm3_tmp_rec

        .area   _CODE
__cpm3_shift7_from_ptr::
        ld      de,#__cpm3_tmp_rec
        ld      b,#4
        call    __cpm3_copy_bytes
        ld      a,(__cpm3_tmp_rec)
        and     #0x7f
        push    af
        ld      b,#7
__cpm3_shift7_loop:
        ld      hl,#__cpm3_tmp_rec + 3
        srl     (hl)
        dec     hl
        rr      (hl)
        dec     hl
        rr      (hl)
        dec     hl
        rr      (hl)
        djnz    __cpm3_shift7_loop
        pop     af
        ret

        ;; Shift __cpm3_tmp_rec left by 7.
