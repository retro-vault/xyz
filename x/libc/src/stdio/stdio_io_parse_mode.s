        ;; stdio_io_parse_mode.s
        ;; Split from stdio_io.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module stdio_io_parse_mode
        .optsdcc -mz80 sdcccall(1)

        .globl  __stdio_io_parse_mode

O_APPEND_HI     .equ 0x04
O_CREAT_HI      .equ 0x01
O_RDWR_V        .equ 0x0002
O_TRUNC_HI      .equ 0x02
O_WRONLY_V      .equ 0x0001
O_WRONLY_APPEND .equ 0x0501
O_WRONLY_TRUNC  .equ 0x0301

        .area   _CODE
__stdio_io_parse_mode::
        ld      a,h
        or      l
        jr      z,__stdio_io_parse_mode_fail
        ld      a,(hl)
        cp      #'r'
        jr      z,__stdio_io_mode_r
        cp      #'w'
        jr      z,__stdio_io_mode_w
        cp      #'a'
        jr      z,__stdio_io_mode_a
        jr      __stdio_io_parse_mode_fail
__stdio_io_mode_r:
        ld      de,#0x0000
        jr      __stdio_io_mode_scan
__stdio_io_mode_w:
        ld      de,#O_WRONLY_TRUNC
        jr      __stdio_io_mode_scan
__stdio_io_mode_a:
        ld      de,#O_WRONLY_APPEND
__stdio_io_mode_scan:
        inc     hl
__stdio_io_mode_scan_loop:
        ld      a,(hl)
        or      a
        jr      z,__stdio_io_parse_mode_ok
        cp      #'+'
        jr      z,__stdio_io_mode_plus
        cp      #'b'
        jr      z,__stdio_io_mode_next
        cp      #'t'
        jr      z,__stdio_io_mode_next
        jr      __stdio_io_parse_mode_fail
__stdio_io_mode_plus:
        ld      e,#O_RDWR_V
__stdio_io_mode_next:
        inc     hl
        jr      __stdio_io_mode_scan_loop
__stdio_io_parse_mode_ok:
        ld      hl,#0x0000
        ret
__stdio_io_parse_mode_fail:
        ld      hl,#0xffff
        ret

        ;; HL = FILE*. Returns HL = 0x00xx on success, 0xFFFF on EOF/error.
