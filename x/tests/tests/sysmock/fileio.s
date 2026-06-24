        ;; fileio.s  (sys backend: sim (in-RAM simulator))
        ;;
        ;; Simple buffer-backed file descriptors for tests. Named buffers can be
        ;; mounted into a tiny table, then opened through POSIX-style open/read/
        ;; write/lseek/close calls. This lets the libc stdio layer exercise real
        ;; file descriptors without needing a filesystem.
        ;;
        ;; Exported test helpers:
        ;;   __sys_file_reset(void)
        ;;   __sys_file_mount(const char *name, void *buf,
        ;;                    unsigned len, unsigned cap)
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih









        .module fileio
        .optsdcc -mz80 sdcccall(1)

        .globl  ___sys_file_reset
        .globl  __sys_file_reset
        .globl  __sys_none_mount_table
        .globl  __sys_none_open_table

MOUNT_COUNT     .equ 4
MOUNT_SIZE      .equ 8
OPEN_COUNT      .equ 4
OPEN_SIZE       .equ 5

        .area   _CODE
__sys_file_reset:
___sys_file_reset::
        xor     a
        ld      hl,#__sys_none_mount_table
        ld      b,#(MOUNT_COUNT * MOUNT_SIZE)
__sys_none_reset_mount_loop:
        ld      (hl),a
        inc     hl
        djnz    __sys_none_reset_mount_loop
        ld      hl,#__sys_none_open_table
        ld      b,#(OPEN_COUNT * OPEN_SIZE)
__sys_none_reset_open_loop:
        ld      (hl),a
        inc     hl
        djnz    __sys_none_reset_open_loop
        ex      de,hl
        ret

