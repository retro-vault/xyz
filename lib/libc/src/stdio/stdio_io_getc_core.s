        ;; stdio_io_getc_core.s
        ;; Split from stdio_io.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module stdio_io_getc_core
        .optsdcc -mz80 sdcccall(1)

        .globl  __stdio_io_getc_core
        .globl  __stdio_io_clear_flags
        .globl  __stdio_io_require_stream
        .globl  _read
        .globl  _getchar

FILE_FLAG_EOF   .equ 0x01
FILE_FLAG_ERR   .equ 0x02
FILE_OFF_PUSHV  .equ 2

        .area   _CODE
__stdio_io_getc_core::
        call    __stdio_io_require_stream
        ld      a,h
        cp      #0xff
        ret     z
        push    hl
        ld      de,#FILE_OFF_PUSHV
        add     hl,de
        ld      a,(hl)
        or      a
        jr      z,__stdio_io_getc_read
        xor     a
        ld      (hl),a
        inc     hl
        ld      l,(hl)
        ld      h,#0x00
        pop     de
        ret
__stdio_io_getc_read:
        pop     hl
        call    __stdio_io_clear_flags
        ld      b,h
        ld      c,l
        ld      a,(hl)
        cp      #3                      ; fd 0/1/2 -> console getchar
        jr      c,__stdio_io_getc_console
        push    bc
        ld      de,#0x0000
        push    de
        ld      hl,#0x0000
        add     hl,sp
        ex      de,hl
        ld      l,a
        ld      h,#0x00
        ld      bc,#0x0001
        push    bc
        call    _read
        pop     bc
        pop     hl
        pop     bc
        ld      a,d
        cp      #0xff
        jr      nz,__stdio_io_getc_count
        ld      a,e
        cp      #0xff
        jr      z,__stdio_io_getc_err
__stdio_io_getc_count:
        ld      a,d
        or      e
        jr      z,__stdio_io_getc_eof
        ld      h,#0x00
        ret
__stdio_io_getc_eof:
        ld      h,b
        ld      l,c
        inc     hl
        ld      a,(hl)
        or      #FILE_FLAG_EOF
        ld      (hl),a
        ld      hl,#0xffff
        ret
__stdio_io_getc_err:
        ld      h,b
        ld      l,c
        inc     hl
        ld      a,(hl)
        or      #FILE_FLAG_ERR
        ld      (hl),a
        ld      hl,#0xffff
        ret

        ;; Console source (fd 0/1/2): read one byte via the platform getchar
        ;; hook.  BC = FILE* on entry.
__stdio_io_getc_console:
        push    bc                      ; preserve FILE* across the hook
        call    _getchar                ; DE = byte, or 0xFFFF on EOF
        pop     bc
        ld      a,d
        cp      #0xff
        jr      nz,__stdio_io_getc_console_ok
        ld      a,e
        cp      #0xff
        jr      z,__stdio_io_getc_console_eof
__stdio_io_getc_console_ok:
        ld      h,#0x00
        ld      l,e
        ret
__stdio_io_getc_console_eof:
        ld      h,b
        ld      l,c
        inc     hl
        ld      a,(hl)
        or      #FILE_FLAG_EOF
        ld      (hl),a
        ld      hl,#0xffff
        ret

        ;; HL = FILE*, E = byte. Returns HL = 0x00xx or 0xFFFF.
