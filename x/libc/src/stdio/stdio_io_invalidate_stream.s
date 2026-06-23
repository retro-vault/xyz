        ;; stdio_io_invalidate_stream.s
        ;; Split from stdio_io.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module stdio_io_invalidate_stream
        .optsdcc -mz80 sdcccall(1)

        .globl  __stdio_io_invalidate_stream
        .globl  __stdio_io_slot_to_tmp_flag
        .globl  __stdio_io_slot_to_tmp_name
        .globl  __stdio_io_stream_slot
        .globl  __stdio_io_file_pool
        .globl  __stdio_io_reset_stream

FILE_FREE_FD    .equ 0xff
FILE_POOL_COUNT .equ 4
TMP_NAME_SIZE   .equ 12

        .area   _DATA
__stdio_io_tmp_flags:
        .db     0,0,0,0
__stdio_io_tmp_names:
        .ds     FILE_POOL_COUNT * TMP_NAME_SIZE

        .area   _CODE
__stdio_io_invalidate_stream::
        ld      a,#FILE_FREE_FD
        jp      __stdio_io_reset_stream

        ;; HL = FILE*. On success return A = pool slot index, carry clear.
        ;; On failure carry is set and A is undefined.
__stdio_io_stream_slot::
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
__stdio_io_slot_to_tmp_flag::
        ld      l,a
        ld      h,#0x00
        ld      de,#__stdio_io_tmp_flags
        add     hl,de
        ret

        ;; A = pool slot index. Return HL = slot-local tmpname buffer.
__stdio_io_slot_to_tmp_name::
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
