        ;; write.s  (sys backend: CP/M 3)
        ;;
        ;; write() — disk block write for file descriptors (>= 3).  Console
        ;; output goes through putchar(), not write().  File writes dispatch
        ;; through __cpm3_write_file_vec, installed by open().

        .module write
        .optsdcc -mz80 sdcccall(1)

        .globl  _write
        .globl  __cpm3_write_file_vec

        .area   _CODE
_write::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      a,h
        or      a
        jp      nz,__cpm3_write_fail    ; invalid (high) fd
        ld      b,h
        ld      c,l
        ld      hl,(__cpm3_write_file_vec)
        ld      a,h
        or      l
        jr      z,__cpm3_write_fail     ; no file opened
        push    hl
        ld      h,b
        ld      l,c
        ret                             ; jump to __cpm3_write_file
__cpm3_write_fail:
        ld      de,#0xffff
        pop     ix
        ret
