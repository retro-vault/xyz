        ;; cpm3_file_vectors.s  (sys backend: CP/M 3)
        ;;
        ;; Link-time decoupling of console I/O from FCB file I/O.
        ;; The read/write/close syscalls reach file descriptors >= 3 only
        ;; through these vectors, and only _open installs them.
        ;; Programs that never open a file therefore never link the FCB
        ;; machinery or its per-descriptor DMA buffers.

        .module cpm3_file_vectors
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
