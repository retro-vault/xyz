        ;; close.s  (sys backend: CP/M 3)
        ;;
        ;; close() syscall. Console descriptors (< 3) succeed inline; file
        ;; descriptors dispatch through __cpm3_close_file_vec so the FCB
        ;; machinery links only when _open is used.

        .module close
        .optsdcc -mz80 sdcccall(1)

        .globl  _close
        .globl  __cpm3_close_file_vec

FD_FILE_BASE    .equ 3

        .area   _CODE
_close::
        ld      a,h
        or      a
        jr      nz,__cpm3_close_fail
        ld      a,l
        cp      #FD_FILE_BASE
        jr      c,__cpm3_close_ok
        ;; fd >= 3: file close through the vector when installed
        ld      b,h
        ld      c,l
        ld      hl,(__cpm3_close_file_vec)
        ld      a,h
        or      l
        jr      z,__cpm3_close_fail
        push    hl
        ld      h,b
        ld      l,c
        ret                             ; jump to __cpm3_close_file
__cpm3_close_ok:
        ld      de,#0x0000
        ret
__cpm3_close_fail:
        ld      de,#0xffff
        ret
