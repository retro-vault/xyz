        ;; file_vectors.s  (sys backend: CP/M 3)
        ;;
        ;; Small read/write/close dispatch cells. Keep these separate from the
        ;; FCB backend so plain read/write/close references do not pull file I/O.

        .module file_vectors
        .optsdcc -mz80 sdcccall(1)

        .globl  __cpm3_write_file_vec
        .globl  __cpm3_read_file_vec
        .globl  __cpm3_close_file_vec

        .area   _DATA
__cpm3_write_file_vec::
        .dw     0
__cpm3_read_file_vec::
        .dw     0
__cpm3_close_file_vec::
        .dw     0
