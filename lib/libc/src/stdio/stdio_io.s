        ;; stdio_io.s
        ;;
        ;; Small fd-backed stdio helpers layered on top of Unix-style read/write
        ;; calls. The current FILE objects are tiny descriptors:
        ;;   +0  fd byte
        ;;   +1  flags (bit0 EOF, bit1 ERR)
        ;;   +2  pushback-valid
        ;;   +3  pushback-char
        ;;
        ;; This is intentionally unbuffered. It gives the libc a real input,
        ;; block-I/O, and basic file-open/seek surface on top of open/read/
        ;; write/lseek/close.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module stdio_io
        .optsdcc -mz80 sdcccall(1)

        .globl  _stdin
        .globl  _stdout
        .globl  _stderr
        .globl  __stdio_stdin_obj
        .globl  __stdio_stdout_obj
        .globl  __stdio_stderr_obj
        .globl  _getchar
        .globl  _fgetc
        .globl  _getc
        .globl  _ungetc
        .globl  _fgets
        .globl  _fread
        .globl  _fwrite
        .globl  _fopen
        .globl  _fclose
        .globl  __stdio_io_freopen_core
        .globl  _fseek
        .globl  _ftell
        .globl  _rewind
        .globl  _fflush
        .globl  _feof
        .globl  _ferror
        .globl  _clearerr
        .globl  _putc
        .globl  __stdio_stdin_handle
        .globl  __stdio_stdout_handle
        .globl  __stdio_stderr_handle
        .globl  _fputc
        .globl  __stdio_io_tmpfile_core
        .globl  _open
        .globl  _close
        .globl  _lseek
        .globl  _read
        .globl  _tmpnam
        .globl  _write
        .globl  __sys_unlink

FILE_FLAG_EOF   .equ 0x01
FILE_FLAG_ERR   .equ 0x02
FILE_FLAG_APP   .equ 0x04

FILE_OFF_FD     .equ 0
FILE_OFF_FLAGS  .equ 1
FILE_OFF_PUSHV  .equ 2
FILE_OFF_PUSHC  .equ 3

FILE_FREE_FD    .equ 0xff
FILE_POOL_COUNT .equ 4
TMP_NAME_SIZE   .equ 12

O_RDONLY_V      .equ 0x0000
O_WRONLY_V      .equ 0x0001
O_RDWR_V        .equ 0x0002
O_CREAT_HI      .equ 0x01
O_TRUNC_HI      .equ 0x02
O_APPEND_HI     .equ 0x04

SEEK_SET_V      .equ 0x0000
SEEK_CUR_V      .equ 0x0001
SEEK_END_V      .equ 0x0002

        .area   _DATA
__stdio_io_tmp_byte:
        .db     0
__stdio_io_stream:
        .dw     0
__stdio_io_size:
        .dw     0
__stdio_io_count:
        .dw     0
__stdio_io_ptr:
        .dw     0
__stdio_io_items:
        .dw     0
__stdio_io_flags:
        .dw     0
__stdio_io_whence:
        .dw     0
__stdio_io_file_pool:
        .db     FILE_FREE_FD,0,0,0
        .db     FILE_FREE_FD,0,0,0
        .db     FILE_FREE_FD,0,0,0
        .db     FILE_FREE_FD,0,0,0
__stdio_io_tmp_slot:
        .db     0
__stdio_io_tmp_flags:
        .db     0,0,0,0
__stdio_io_tmp_names:
        .ds     FILE_POOL_COUNT * TMP_NAME_SIZE

        .area   _CONST
__stdio_io_tmpfile_mode:
        .ascii  "w+b\0"

        .area   _CODE

        ;; HL = FILE*. Return HL = 0xFFFF on null, otherwise leave HL as-is.
__stdio_io_require_stream:
        ld      a,h
        or      l
        ret     nz
        ld      hl,#0xffff
        ret

        ;; HL = FILE*. Clear EOF+ERR bits.
__stdio_io_clear_flags:
        push    hl
        ld      de,#FILE_OFF_FLAGS
        add     hl,de
        ld      a,(hl)
        and     #~(FILE_FLAG_EOF | FILE_FLAG_ERR)
        ld      (hl),a
        pop     hl
        ret

        ;; HL = FILE*. A = flag bits to OR in.
__stdio_io_set_flags:
        push    af
        push    hl
        ld      de,#FILE_OFF_FLAGS
        add     hl,de
        pop     de
        pop     af
        or      (hl)
        ld      (hl),a
        ex      de,hl
        ret

        ;; HL = FILE*, A = fd byte. Reset flags/pushback and install the fd.
__stdio_io_reset_stream:
        ld      (hl),a
        inc     hl
        xor     a
        ld      (hl),a
        inc     hl
        ld      (hl),a
        inc     hl
        ld      (hl),a
        ret

        ;; HL = FILE*. Mark the slot free.
__stdio_io_invalidate_stream:
        ld      a,#FILE_FREE_FD
        jp      __stdio_io_reset_stream

        ;; HL = FILE*. On success return A = pool slot index, carry clear.
        ;; On failure carry is set and A is undefined.
__stdio_io_stream_slot:
        ld      de,#__stdio_io_file_pool
        ld      c,#0
        ld      b,#FILE_POOL_COUNT
__stdio_io_stream_slot_loop:
        ld      a,h
        cp      d
        jr      nz,__stdio_io_stream_slot_next
        ld      a,l
        cp      e
        jr      z,__stdio_io_stream_slot_hit
__stdio_io_stream_slot_next:
        inc     c
        ex      de,hl
        ld      de,#4
        add     hl,de
        ex      de,hl
        djnz    __stdio_io_stream_slot_loop
        scf
        ret
__stdio_io_stream_slot_hit:
        ld      a,c
        or      a
        ret

        ;; A = pool slot index. Return HL = temp-flag byte.
__stdio_io_slot_to_tmp_flag:
        ld      l,a
        ld      h,#0x00
        ld      de,#__stdio_io_tmp_flags
        add     hl,de
        ret

        ;; A = pool slot index. Return HL = slot-local tmpname buffer.
__stdio_io_slot_to_tmp_name:
        ld      hl,#__stdio_io_tmp_names
        or      a
        ret     z
        ld      b,a
__stdio_io_slot_to_tmp_name_loop:
        ld      de,#TMP_NAME_SIZE
        add     hl,de
        djnz    __stdio_io_slot_to_tmp_name_loop
        ret

        ;; HL = FILE*. Clear temporary-file metadata for a pooled stream.
__stdio_io_tmp_clear:
        push    hl
        call    __stdio_io_stream_slot
        jr      c,__stdio_io_tmp_clear_done
        call    __stdio_io_slot_to_tmp_flag
        xor     a
        ld      (hl),a
__stdio_io_tmp_clear_done:
        pop     hl
        ret

        ;; HL = FILE*. If this is a tmpfile stream, unlink its slot-local name
        ;; after the descriptor has been closed and clear the tmp flag.
__stdio_io_tmp_cleanup:
        push    hl
        call    __stdio_io_stream_slot
        jr      c,__stdio_io_tmp_cleanup_done
        ld      (__stdio_io_tmp_slot),a
        call    __stdio_io_slot_to_tmp_flag
        ld      a,(hl)
        or      a
        jr      z,__stdio_io_tmp_cleanup_done
        xor     a
        ld      (hl),a
        ld      a,(__stdio_io_tmp_slot)
        call    __stdio_io_slot_to_tmp_name
        call    __sys_unlink
__stdio_io_tmp_cleanup_done:
        pop     hl
        ret

        ;; Return HL = free FILE slot or 0.
__stdio_io_alloc_stream:
        ld      hl,#__stdio_io_file_pool
        ld      b,#FILE_POOL_COUNT
__stdio_io_alloc_stream_loop:
        ld      a,(hl)
        cp      #FILE_FREE_FD
        ret     z
        ld      de,#4
        add     hl,de
        djnz    __stdio_io_alloc_stream_loop
        ld      hl,#0x0000
        ret

        ;; Parse fopen() mode text at HL. On success store flags in
        ;; __stdio_io_flags and return HL = 0. On failure return 0xFFFF.
__stdio_io_parse_mode:
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
        xor     a
        ld      (__stdio_io_flags),a
        ld      (__stdio_io_flags + 1),a
        jr      __stdio_io_mode_scan
__stdio_io_mode_w:
        ld      a,#O_WRONLY_V
        ld      (__stdio_io_flags),a
        ld      a,#(O_CREAT_HI | O_TRUNC_HI)
        ld      (__stdio_io_flags + 1),a
        jr      __stdio_io_mode_scan
__stdio_io_mode_a:
        ld      a,#O_WRONLY_V
        ld      (__stdio_io_flags),a
        ld      a,#(O_CREAT_HI | O_APPEND_HI)
        ld      (__stdio_io_flags + 1),a
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
        ld      a,#O_RDWR_V
        ld      (__stdio_io_flags),a
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
__stdio_io_getc_core:
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
        push    hl
        ld      a,(hl)
        ld      l,a
        ld      h,#0x00
        ld      de,#__stdio_io_tmp_byte
        ld      bc,#0x0001
        push    bc
        call    _read
        pop     bc
        pop     hl
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
        ld      a,(__stdio_io_tmp_byte)
        ld      l,a
        ld      h,#0x00
        ret
__stdio_io_getc_eof:
        inc     hl
        ld      a,(hl)
        or      #FILE_FLAG_EOF
        ld      (hl),a
        ld      hl,#0xffff
        ret
__stdio_io_getc_err:
        inc     hl
        ld      a,(hl)
        or      #FILE_FLAG_ERR
        ld      (hl),a
        ld      hl,#0xffff
        ret

        ;; HL = FILE*, E = byte. Returns HL = 0x00xx or 0xFFFF.
__stdio_io_putc_core:
        call    __stdio_io_require_stream
        ld      a,h
        cp      #0xff
        ret     z
        push    de
        call    __stdio_io_clear_flags
        pop     de
        push    hl
        ld      a,e
        ld      (__stdio_io_tmp_byte),a
        ld      a,(hl)
        ld      l,a
        ld      h,#0x00
        ld      de,#__stdio_io_tmp_byte
        ld      bc,#0x0001
        push    bc
        call    _write
        pop     bc
        pop     hl
        ld      a,d
        cp      #0xff
        jr      nz,__stdio_io_putc_count
        ld      a,e
        cp      #0xff
        jr      z,__stdio_io_putc_err
__stdio_io_putc_count:
        ld      a,d
        or      e
        jr      z,__stdio_io_putc_err
        ld      a,(__stdio_io_tmp_byte)
        ld      l,a
        ld      h,#0x00
        ret
__stdio_io_putc_err:
        inc     hl
        ld      a,(hl)
        or      #FILE_FLAG_ERR
        ld      (hl),a
        ld      hl,#0xffff
        ret

_getchar::
        ld      hl,(_stdin)
        call    __stdio_io_getc_core
        push    hl
        pop     de
        ret

__stdio_stdin_handle::
        ld      hl,#__stdio_stdin_obj
        ld      (_stdin),hl
        xor     a
        push    hl
        call    __stdio_io_reset_stream
        pop     de
        ret

__stdio_stdout_handle::
        ld      hl,#__stdio_stdout_obj
        ld      (_stdout),hl
        ld      a,#1
        push    hl
        call    __stdio_io_reset_stream
        pop     de
        ret

__stdio_stderr_handle::
        ld      hl,#__stdio_stderr_obj
        ld      (_stderr),hl
        ld      a,#2
        push    hl
        call    __stdio_io_reset_stream
        pop     de
        ret

_fgetc::
        call    __stdio_io_getc_core
        push    hl
        pop     de
        ret

_getc::
        jp      _fgetc

_ungetc::
        push    hl
        ex      de,hl
        call    __stdio_io_require_stream
        ld      a,h
        cp      #0xff
        jr      z,__stdio_io_ungetc_fail_popchar
        push    hl
        ld      de,#FILE_OFF_PUSHV
        add     hl,de
        ld      a,(hl)
        pop     hl
        or      a
        jr      nz,__stdio_io_ungetc_fail_popchar
        pop     de
        ld      a,e
        cp      #0xff
        jr      z,__stdio_io_ungetc_fail
        push    de
        call    __stdio_io_clear_flags
        pop     de
        ld      bc,#FILE_OFF_PUSHV
        add     hl,bc
        ld      a,#1
        ld      (hl),a
        inc     hl
        ld      a,e
        ld      (hl),a
        ld      l,a
        ld      h,#0x00
        push    hl
        pop     de
        ret
__stdio_io_ungetc_fail_popchar:
        pop     bc
__stdio_io_ungetc_fail:
        ld      hl,#0xffff
        push    hl
        pop     de
        ret

_fgets::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      (__stdio_io_ptr),hl
        ld      a,h
        or      l
        jr      z,__stdio_io_fgets_fail
        ld      c,e
        ld      b,d
        ld      a,b
        or      c
        jr      z,__stdio_io_fgets_fail
        ld      l,4(ix)
        ld      h,5(ix)
        call    __stdio_io_require_stream
        ld      a,h
        cp      #0xff
        jr      z,__stdio_io_fgets_fail
        ld      (__stdio_io_stream),hl
        ld      hl,(__stdio_io_ptr)
        dec     bc
        jr      z,__stdio_io_fgets_empty
        xor     a
        ld      (__stdio_io_items),a
        ld      (__stdio_io_items + 1),a
__stdio_io_fgets_loop:
        push    bc
        ld      bc,(__stdio_io_stream)
        ld      h,b
        ld      l,c
        call    __stdio_io_getc_core
        pop     bc
        ld      a,h
        cp      #0xff
        jr      z,__stdio_io_fgets_eof
        ld      de,(__stdio_io_ptr)
        ld      a,l
        ld      (de),a
        inc     de
        ld      (__stdio_io_ptr),de
        ld      hl,(__stdio_io_items)
        inc     hl
        ld      (__stdio_io_items),hl
        cp      #'\n'
        jr      z,__stdio_io_fgets_done_store
        dec     bc
        ld      a,b
        or      c
        jr      nz,__stdio_io_fgets_loop
__stdio_io_fgets_done_store:
        xor     a
        ld      de,(__stdio_io_ptr)
        ld      (de),a
        ld      hl,(__stdio_io_ptr)
        ld      de,(__stdio_io_items)
        ld      a,d
        or      e
        jr      nz,__stdio_io_fgets_return_ptr
        ld      hl,#0x0000
        pop     ix
        ret
__stdio_io_fgets_return_ptr:
        ld      hl,(__stdio_io_ptr)
        ld      de,(__stdio_io_items)
        or      a
        sbc     hl,de
        push    hl
        pop     de
        pop     ix
        ret
__stdio_io_fgets_eof:
        ld      hl,(__stdio_io_items)
        ld      a,h
        or      l
        jr      nz,__stdio_io_fgets_done_store
__stdio_io_fgets_fail:
        ld      hl,#0x0000
        push    hl
        pop     de
        pop     ix
        ret
__stdio_io_fgets_empty:
        xor     a
        ld      (hl),a
        ld      hl,(__stdio_io_ptr)
        push    hl
        pop     de
        pop     ix
        ret

        ;; Nested byte loop: return number of complete items moved in HL.
_fread::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      (__stdio_io_ptr),hl
        ld      c,e
        ld      b,d
        ld      (__stdio_io_size),bc
        ld      c,4(ix)
        ld      b,5(ix)
        ld      (__stdio_io_count),bc
        ld      l,6(ix)
        ld      h,7(ix)
        call    __stdio_io_require_stream
        ld      a,h
        cp      #0xff
        jp      z,__stdio_io_rw_zero
        ld      (__stdio_io_stream),hl
        xor     a
        ld      (__stdio_io_items),a
        ld      (__stdio_io_items + 1),a
        ld      bc,(__stdio_io_count)
__stdio_io_fread_item:
        ld      a,b
        or      c
        jr      z,__stdio_io_fread_done
        push    bc
        ld      bc,(__stdio_io_size)
__stdio_io_fread_byte:
        ld      a,b
        or      c
        jr      z,__stdio_io_fread_item_done
        push    bc
        ld      hl,(__stdio_io_stream)
        call    __stdio_io_getc_core
        pop     bc
        ld      a,h
        cp      #0xff
        jr      z,__stdio_io_fread_abort
        ld      de,(__stdio_io_ptr)
        ld      a,l
        ld      (de),a
        inc     de
        ld      (__stdio_io_ptr),de
        dec     bc
        jr      __stdio_io_fread_byte
__stdio_io_fread_item_done:
        pop     bc
        dec     bc
        ld      (__stdio_io_count),bc
        ld      hl,(__stdio_io_items)
        inc     hl
        ld      (__stdio_io_items),hl
        jr      __stdio_io_fread_item
__stdio_io_fread_abort:
        pop     bc
__stdio_io_fread_done:
        ld      hl,(__stdio_io_items)
        push    hl
        pop     de
        pop     ix
        ret

_fwrite::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      (__stdio_io_ptr),hl
        ld      c,e
        ld      b,d
        ld      (__stdio_io_size),bc
        ld      c,4(ix)
        ld      b,5(ix)
        ld      (__stdio_io_count),bc
        ld      l,6(ix)
        ld      h,7(ix)
        call    __stdio_io_require_stream
        ld      a,h
        cp      #0xff
        jp      z,__stdio_io_rw_zero
        ld      (__stdio_io_stream),hl
        xor     a
        ld      (__stdio_io_items),a
        ld      (__stdio_io_items + 1),a
        ld      bc,(__stdio_io_count)
__stdio_io_fwrite_item:
        ld      a,b
        or      c
        jr      z,__stdio_io_fwrite_done
        push    bc
        ld      bc,(__stdio_io_size)
__stdio_io_fwrite_byte:
        ld      a,b
        or      c
        jr      z,__stdio_io_fwrite_item_done
        push    bc
        ld      hl,(__stdio_io_ptr)
        ld      e,(hl)
        inc     hl
        ld      (__stdio_io_ptr),hl
        ld      hl,(__stdio_io_stream)
        call    __stdio_io_putc_core
        pop     bc
        ld      a,h
        cp      #0xff
        jr      z,__stdio_io_fwrite_abort
        dec     bc
        jr      __stdio_io_fwrite_byte
__stdio_io_fwrite_item_done:
        pop     bc
        dec     bc
        ld      (__stdio_io_count),bc
        ld      hl,(__stdio_io_items)
        inc     hl
        ld      (__stdio_io_items),hl
        jr      __stdio_io_fwrite_item
__stdio_io_fwrite_abort:
        pop     bc
__stdio_io_fwrite_done:
        ld      hl,(__stdio_io_items)
        push    hl
        pop     de
        pop     ix
        ret

__stdio_io_rw_zero:
        ld      hl,#0x0000
        pop     ix
        ret

_fflush::
        ld      hl,#0x0000
        push    hl
        pop     de
        ret

_feof::
        call    __stdio_io_require_stream
        ld      a,h
        cp      #0xff
        jr      z,__stdio_io_flag_false
        ld      de,#FILE_OFF_FLAGS
        add     hl,de
        ld      a,(hl)
        and     #FILE_FLAG_EOF
        jr      z,__stdio_io_flag_false
        ld      hl,#0x0001
        push    hl
        pop     de
        ret
__stdio_io_flag_false:
        ld      hl,#0x0000
        push    hl
        pop     de
        ret

_ferror::
        call    __stdio_io_require_stream
        ld      a,h
        cp      #0xff
        jr      z,__stdio_io_err_false
        ld      de,#FILE_OFF_FLAGS
        add     hl,de
        ld      a,(hl)
        and     #FILE_FLAG_ERR
        jr      z,__stdio_io_err_false
        ld      hl,#0x0001
        push    hl
        pop     de
        ret
__stdio_io_err_false:
        ld      hl,#0x0000
        push    hl
        pop     de
        ret

_clearerr::
        call    __stdio_io_require_stream
        ld      a,h
        cp      #0xff
        jr      z,__stdio_io_clearerr_done
        call    __stdio_io_clear_flags
__stdio_io_clearerr_done:
        ret

_putc::
        jp      _fputc

_fopen::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      (__stdio_io_ptr),hl
        push    de
        pop     hl
        call    __stdio_io_parse_mode
        ld      a,h
        cp      #0xff
        jr      z,__stdio_io_fopen_fail
        ld      hl,(__stdio_io_ptr)
        ld      de,(__stdio_io_flags)
        ld      bc,#0x0000
        push    bc
        call    _open
        pop     bc
        ld      a,d
        cp      #0xff
        jr      nz,__stdio_io_fopen_gotfd
        ld      a,e
        cp      #0xff
        jr      z,__stdio_io_fopen_fail
__stdio_io_fopen_gotfd:
        push    de
        call    __stdio_io_alloc_stream
        pop     de
        ld      a,h
        or      l
        jr      nz,__stdio_io_fopen_have_slot
        ld      l,e
        ld      h,d
        call    _close
        jr      __stdio_io_fopen_fail
__stdio_io_fopen_have_slot:
        ld      a,e
        push    hl
        call    __stdio_io_reset_stream
        pop     hl
        push    hl
        pop     de
        pop     ix
        ret
__stdio_io_fopen_fail:
        ld      hl,#0x0000
        push    hl
        pop     de
        pop     ix
        ret

_fclose::
        call    __stdio_io_require_stream
        ld      a,h
        cp      #0xff
        jr      z,__stdio_io_fclose_fail
        push    hl
        ld      a,(hl)
        cp      #FILE_FREE_FD
        jr      z,__stdio_io_fclose_fail_pop
        cp      #3
        jr      c,__stdio_io_fclose_fail_pop
        ld      l,a
        ld      h,#0x00
        call    _close
        ld      a,d
        cp      #0xff
        jr      nz,__stdio_io_fclose_ok
        ld      a,e
        cp      #0xff
        jr      z,__stdio_io_fclose_fail_pop
__stdio_io_fclose_ok:
        pop     hl
        call    __stdio_io_tmp_cleanup
        call    __stdio_io_invalidate_stream
        ld      hl,#0x0000
        push    hl
        pop     de
        ret
__stdio_io_fclose_fail_pop:
        pop     hl
__stdio_io_fclose_fail:
        ld      hl,#0xffff
        push    hl
        pop     de
        ret

        ;; tmpfile(): allocate a pooled FILE slot, generate a slot-local name,
        ;; open it with w+b semantics, and remember that fclose() must unlink
        ;; the backing file after closing the descriptor.
__stdio_io_tmpfile_core::
        call    __stdio_io_alloc_stream
        ld      a,h
        or      l
        ret     z
        ld      (__stdio_io_stream),hl
        call    __stdio_io_stream_slot
        jr      c,__stdio_io_tmpfile_fail
        ld      (__stdio_io_tmp_slot),a
        call    __stdio_io_slot_to_tmp_name
        call    _tmpnam
        ld      a,#(O_CREAT_HI | O_TRUNC_HI)
        ld      d,a
        ld      e,#O_RDWR_V
        ld      bc,#0x0000
        push    bc
        call    _open
        pop     bc
        ld      a,d
        cp      #0xff
        jr      nz,__stdio_io_tmpfile_have_fd
        ld      a,e
        cp      #0xff
        jr      z,__stdio_io_tmpfile_fail
__stdio_io_tmpfile_have_fd:
        ld      hl,(__stdio_io_stream)
        ld      a,e
        push    hl
        call    __stdio_io_reset_stream
        pop     hl
        ld      a,(__stdio_io_tmp_slot)
        call    __stdio_io_slot_to_tmp_flag
        ld      (hl),#1
        ld      hl,(__stdio_io_stream)
        push    hl
        pop     de
        ret
__stdio_io_tmpfile_fail:
        ld      hl,(__stdio_io_stream)
        call    __stdio_io_tmp_clear
        call    __stdio_io_invalidate_stream
        ld      hl,#0x0000
        push    hl
        pop     de
        ret

        ;; freopen(path, mode, stream): close the current descriptor, clear any
        ;; tmpfile cleanup state, then reopen the supplied stream object in-place.
__stdio_io_freopen_core::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      (__stdio_io_ptr),hl
        push    de
        pop     hl
        call    __stdio_io_parse_mode
        ld      a,h
        cp      #0xff
        jr      z,__stdio_io_freopen_fail
        ld      l,4(ix)
        ld      h,5(ix)
        call    __stdio_io_require_stream
        ld      a,h
        cp      #0xff
        jr      z,__stdio_io_freopen_fail
        ld      (__stdio_io_stream),hl
        push    hl
        ld      a,(hl)
        cp      #FILE_FREE_FD
        jr      z,__stdio_io_freopen_skip_close
        ld      l,a
        ld      h,#0x00
        call    _close
        pop     hl
        call    __stdio_io_tmp_cleanup
        jr      __stdio_io_freopen_open
__stdio_io_freopen_skip_close:
        pop     hl
        call    __stdio_io_tmp_clear
__stdio_io_freopen_open:
        ld      hl,(__stdio_io_ptr)
        ld      de,(__stdio_io_flags)
        ld      bc,#0x0000
        push    bc
        call    _open
        pop     bc
        ld      a,d
        cp      #0xff
        jr      nz,__stdio_io_freopen_gotfd
        ld      a,e
        cp      #0xff
        jr      z,__stdio_io_freopen_open_fail
__stdio_io_freopen_gotfd:
        ld      hl,(__stdio_io_stream)
        ld      a,e
        push    hl
        call    __stdio_io_reset_stream
        pop     hl
        push    hl
        pop     de
        pop     ix
        ret
__stdio_io_freopen_open_fail:
        ld      hl,(__stdio_io_stream)
        call    __stdio_io_invalidate_stream
__stdio_io_freopen_fail:
        ld      hl,#0x0000
        push    hl
        pop     de
        pop     ix
        ret

_fseek::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __stdio_io_require_stream
        ld      a,h
        cp      #0xff
        jr      z,__stdio_io_fseek_fail
        ld      (__stdio_io_stream),hl
        ld      a,(hl)
        cp      #FILE_FREE_FD
        jr      z,__stdio_io_fseek_err
        ld      l,a
        ld      h,#0x00
        ld      c,8(ix)
        ld      b,9(ix)
        push    bc
        ld      c,6(ix)
        ld      b,7(ix)
        push    bc
        ld      c,4(ix)
        ld      b,5(ix)
        push    bc
        call    _lseek
        pop     bc
        pop     bc
        pop     bc
        ld      a,h
        cp      #0xff
        jr      nz,__stdio_io_fseek_ok
        ld      a,l
        cp      #0xff
        jr      nz,__stdio_io_fseek_ok
        ld      a,d
        cp      #0xff
        jr      nz,__stdio_io_fseek_ok
        ld      a,e
        cp      #0xff
        jr      z,__stdio_io_fseek_err
__stdio_io_fseek_ok:
        ld      hl,(__stdio_io_stream)
        ld      a,(hl)
        call    __stdio_io_reset_stream
        ld      hl,#0x0000
        push    hl
        pop     de
        pop     ix
        ret
__stdio_io_fseek_err:
        ld      hl,(__stdio_io_stream)
        ld      a,#FILE_FLAG_ERR
        call    __stdio_io_set_flags
__stdio_io_fseek_fail:
        ld      hl,#0xffff
        push    hl
        pop     de
        pop     ix
        ret

_ftell::
        call    __stdio_io_require_stream
        ld      a,h
        cp      #0xff
        jr      z,__stdio_io_ftell_fail
        ld      (__stdio_io_stream),hl
        ld      a,(hl)
        cp      #FILE_FREE_FD
        jr      z,__stdio_io_ftell_err
        ld      l,a
        ld      h,#0x00
        ld      bc,#SEEK_CUR_V
        push    bc
        ld      bc,#0x0000
        push    bc
        push    bc
        call    _lseek
        pop     bc
        pop     bc
        pop     bc
        ld      a,h
        cp      #0xff
        jr      nz,__stdio_io_ftell_swap
        ld      a,l
        cp      #0xff
        jr      nz,__stdio_io_ftell_swap
        ld      a,d
        cp      #0xff
        jr      nz,__stdio_io_ftell_swap
        ld      a,e
        cp      #0xff
        jr      z,__stdio_io_ftell_err
__stdio_io_ftell_swap:
        push    hl
        push    de
        ld      hl,(__stdio_io_stream)
        ld      de,#FILE_OFF_PUSHV
        add     hl,de
        ld      a,(hl)
        pop     de
        pop     hl
        or      a
        jr      z,__stdio_io_ftell_done
        dec     de
        ld      a,d
        cp      #0xff
        jr      nz,__stdio_io_ftell_done
        dec     hl
__stdio_io_ftell_done:
        ret
__stdio_io_ftell_err:
        ld      hl,(__stdio_io_stream)
        ld      a,#FILE_FLAG_ERR
        call    __stdio_io_set_flags
__stdio_io_ftell_fail:
        ld      hl,#0xffff
        ld      de,#0xffff
        ret

_rewind::
        call    __stdio_io_require_stream
        ld      a,h
        cp      #0xff
        jr      z,__stdio_io_rewind_done
        ld      (__stdio_io_stream),hl
        ld      a,(hl)
        cp      #FILE_FREE_FD
        jr      z,__stdio_io_rewind_err
        ld      l,a
        ld      h,#0x00
        ld      bc,#SEEK_SET_V
        push    bc
        ld      bc,#0x0000
        push    bc
        push    bc
        call    _lseek
        pop     bc
        pop     bc
        pop     bc
        ld      a,h
        cp      #0xff
        jr      nz,__stdio_io_rewind_ok
        ld      a,l
        cp      #0xff
        jr      nz,__stdio_io_rewind_ok
        ld      a,d
        cp      #0xff
        jr      nz,__stdio_io_rewind_ok
        ld      a,e
        cp      #0xff
        jr      z,__stdio_io_rewind_err
__stdio_io_rewind_ok:
        ld      hl,(__stdio_io_stream)
        ld      a,(hl)
        call    __stdio_io_reset_stream
        jr      __stdio_io_rewind_done
__stdio_io_rewind_err:
        ld      hl,(__stdio_io_stream)
        ld      a,#FILE_FLAG_ERR
        call    __stdio_io_set_flags
__stdio_io_rewind_done:
        ret
