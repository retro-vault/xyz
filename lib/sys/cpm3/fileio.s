        ;; fileio.s  (sys backend: CP/M 3)
        ;;
        ;; CP/M 3 file-descriptor backend for the xcc libc.  This bridges the
        ;; libc's small Unix-like open/read/write/lseek/close/remove/rename
        ;; surface onto BDOS FCB calls, keeping one private DMA buffer per open
        ;; file and committing exact file lengths on flush/close.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih












        .module fileio
        .optsdcc -mz80 sdcccall(1)

        .globl  __cpm3_console_getchar
        .globl  __cpm3_copy_bytes
        .globl  __cpm3_tmp_rec
        .globl  __cpm3_tmp_rec2

BDOS            .equ 5
C_READ          .equ 1

        .area   _CODE
__cpm3_copy_tmprec_to_tmprec2:
        ld      hl,#__cpm3_tmp_rec
        ld      de,#__cpm3_tmp_rec2
        ld      b,#4
        jp      __cpm3_copy_bytes

        ;; Build __cpm3_tmp_rec = file size represented by __cpm3_tmp_fcb's
        ;; random record count plus last-record byte count.
__cpm3_bin2bcd_a:
        ld      e,a
        xor     a
__cpm3_bin2bcd_tens:
        ld      d,a
        ld      a,e
        cp      #10
        jr      c,__cpm3_bin2bcd_done
        sub     #10
        ld      e,a
        ld      a,d
        add     a,#0x10
        jr      __cpm3_bin2bcd_tens
__cpm3_bin2bcd_done:
        ld      a,d
        add     a,e
        ret

        ;; Return one console byte in A.
        ;; BDOS clobbers BC/DE/HL; preserve them for callers that keep
        ;; loop state in registers.
__cpm3_console_getchar::
        push    ix
        push    iy
        push    bc
        push    de
        push    hl
        ld      c,#C_READ
        call    BDOS
        pop     hl
        pop     de
        pop     bc
        pop     iy
        pop     ix
        ret

        ;; Emit console byte in E.
        ;; BDOS clobbers BC/DE/HL; preserve them for callers that keep
        ;; loop state in registers.
