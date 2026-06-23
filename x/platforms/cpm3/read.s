        ;; read.s  (sys backend: CP/M 3)
        ;;
        ;; read() — disk block read for file descriptors (>= 3).  Console
        ;; input goes through getchar(), not read().  File reads dispatch
        ;; through __cpm3_read_file_vec, installed by open().

        .module read
        .optsdcc -mz80 sdcccall(1)

        .globl  _read
        .globl  __cpm3_read_file_vec

        .area   _CODE
_read::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      a,h
        or      a
        jp      nz,__cpm3_read_fail     ; invalid (high) fd
        ld      b,h
        ld      c,l
        ld      hl,(__cpm3_read_file_vec)
        ld      a,h
        or      l
        jr      z,__cpm3_read_fail      ; no file opened
        push    hl
        ld      h,b
        ld      l,c
        ret                             ; jump to __cpm3_read_file
__cpm3_read_fail:
        ld      de,#0xffff
        pop     ix
        ret
