        ;; floppy.inc
        ;;
        ;; Public constants and register ABI for libfloppy.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-08-10   tstih

        ;; Public status values. Every public routine returns the status in A;
        ;; zero also sets Z, while every error is non-zero and sets NZ.
        .equ    FLOPPY_OK,                  0
        .equ    FLOPPY_ERR_ARGUMENT,        1
        .equ    FLOPPY_ERR_TIMEOUT,         2
        .equ    FLOPPY_ERR_SEEK,            3
        .equ    FLOPPY_ERR_IO,              4

        ;; Controller ports.
        .equ    FLOPPY_PORT_MOTOR,          0x98
        .equ    FLOPPY_PORT_DMA,            0xc0
        .equ    FLOPPY_PORT_VECTOR,         0xe8
        .equ    FLOPPY_PORT_MSR,            0xf0
        .equ    FLOPPY_PORT_DATA,           0xf1

        ;; The FDC vector latch is programmed to this even IM2 vector.
        .equ    FLOPPY_VECTOR,              0xe8
