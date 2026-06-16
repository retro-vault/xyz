        ;; cpm3_inc_tmprec.s
        ;; Split from fileio.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module cpm3_inc_tmprec
        .optsdcc -mz80 sdcccall(1)

        .globl  __cpm3_sync_iy
        .globl  __cpm3_entry_fcb_ptr_iy
        .globl  __cpm3_flush_iy
        .globl  __cpm3_shift7_from_ptr
        .globl  __cpm3_tmp_rec

ACC_MASK        .equ 3
BDOS            .equ 5
BDOS_SUCCESS    .equ 0
FCB_OFF_F6ATTR  .equ 6
FCB_OFF_RREC0   .equ 33
FCB_OFF_RREC1   .equ 34
FCB_OFF_RREC2   .equ 35
FCB_OFF_SEQREQ  .equ 32
FD_OFF_FCB      .equ 17
FD_OFF_FLAGS    .equ 1
FD_OFF_FSIZE0   .equ 9
F_ATTRIB        .equ 30
F_TRUNCATE      .equ 99

        .area   _CODE
__cpm3_inc_tmprec:
        ld      hl,#__cpm3_tmp_rec
        ld      a,(hl)
        inc     a
        ld      (hl),a
        ret     nz
        inc     hl
        ld      a,(hl)
        inc     a
        ld      (hl),a
        ret     nz
        inc     hl
        ld      a,(hl)
        inc     a
        ld      (hl),a
        ret     nz
        inc     hl
        ld      a,(hl)
        inc     a
        ld      (hl),a
        ret

        ;; Clear __cpm3_tmp_rec.
__cpm3_sync_iy::
        push    bc
        push    de
        push    hl
        call    __cpm3_sync_iy_impl
        pop     hl
        pop     de
        pop     bc
        ret
__cpm3_sync_iy_impl:
        call    __cpm3_flush_iy
        ret     nz
        ld      a,FD_OFF_FLAGS(iy)
        and     #ACC_MASK
        ret     z
        push    iy
        pop     hl
        ld      de,#FD_OFF_FSIZE0
        add     hl,de
        call    __cpm3_shift7_from_ptr
        ld      b,a                     ; B = last-record byte count
        or      a
        jr      z,__cpm3_sync_no_ceil
        call    __cpm3_inc_tmprec
__cpm3_sync_no_ceil:
        ld      a,(__cpm3_tmp_rec)
        ld      FD_OFF_FCB + FCB_OFF_RREC0(iy),a
        ld      a,(__cpm3_tmp_rec + 1)
        ld      FD_OFF_FCB + FCB_OFF_RREC1(iy),a
        ld      a,(__cpm3_tmp_rec + 2)
        ld      FD_OFF_FCB + FCB_OFF_RREC2(iy),a
        call    __cpm3_entry_fcb_ptr_iy
        ex      de,hl
        push    ix
        push    iy
        push    bc                      ; keep remainder across BDOS
        ld      c,#F_TRUNCATE
        call    BDOS
        pop     bc
        pop     iy
        pop     ix
        cp      #BDOS_SUCCESS
        jr      nz,__cpm3_sync_truncate_unsupported
        ld      a,b
        or      a
        ret     z
        ld      a,b
        ld      FD_OFF_FCB + FCB_OFF_SEQREQ(iy),a
        ld      a,FD_OFF_FCB + FCB_OFF_F6ATTR(iy)
        or      #0x80
        ld      FD_OFF_FCB + FCB_OFF_F6ATTR(iy),a
        call    __cpm3_entry_fcb_ptr_iy
        ex      de,hl
        push    ix
        push    iy
        ld      c,#F_ATTRIB
        call    BDOS
        pop     iy
        pop     ix
        push    af                      ; keep BDOS status across restore
        ld      a,FD_OFF_FCB + FCB_OFF_F6ATTR(iy)
        and     #0x7f
        ld      FD_OFF_FCB + FCB_OFF_F6ATTR(iy),a
        pop     af
        cp      #BDOS_SUCCESS
        jr      nz,__cpm3_sync_fail
        xor     a
        ret
__cpm3_sync_truncate_unsupported:
        ;; Hosts without BDOS 99 (e.g. CP/M 2.2-style emulators) keep the
        ;; record-rounded length; that is not a close failure.
        xor     a
        ret
__cpm3_sync_fail:
        ld      a,#1
        ret

        ;; A -> BCD in A.
