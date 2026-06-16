        ;; stdio_alloc_ctx.s
        ;; Split from printf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module stdio_alloc_ctx
        .optsdcc -mz80 sdcccall(1)

        .globl  __stdio_alloc_ctx
        .globl  __stdio_emit_a
        .globl  __stdio_load_count_hl
        .globl  __stdio_load_sink_ptr_hl
        .globl  __stdio_set_count_zero
        .globl  __stdio_store_sink_ptr_hl
        .globl  __stdio_store_sink_room_hl
        .globl  _putchar

CTX_COUNT       .equ 18
CTX_EMIT_BYTE   .equ 72
CTX_SINK_FD     .equ 2
CTX_SINK_KIND   .equ 0
CTX_SINK_PTR    .equ 14
CTX_SINK_ROOM   .equ 16
CTX_SIZE        .equ 73
SINK_STRING     .equ 0x01

        .area   _CODE
__stdio_alloc_ctx::
        pop     de
        ld      hl,#-CTX_SIZE
        add     hl,sp
        ld      sp,hl
        ld      iy,#0
        add     iy,sp
        push    de
        ret

__stdio_load_sink_ptr_hl::
        ld      a,CTX_SINK_PTR(iy)
        ld      l,a
        ld      a,CTX_SINK_PTR+1(iy)
        ld      h,a
        ret

__stdio_store_sink_ptr_hl::
        ld      a,l
        ld      CTX_SINK_PTR(iy),a
        ld      a,h
        ld      CTX_SINK_PTR+1(iy),a
        ret

__stdio_load_sink_room_hl:
        ld      a,CTX_SINK_ROOM(iy)
        ld      l,a
        ld      a,CTX_SINK_ROOM+1(iy)
        ld      h,a
        ret

__stdio_store_sink_room_hl::
        ld      a,l
        ld      CTX_SINK_ROOM(iy),a
        ld      a,h
        ld      CTX_SINK_ROOM+1(iy),a
        ret

__stdio_load_count_hl::
        ld      a,CTX_COUNT(iy)
        ld      l,a
        ld      a,CTX_COUNT+1(iy)
        ld      h,a
        ret

__stdio_store_count_hl:
        ld      a,l
        ld      CTX_COUNT(iy),a
        ld      a,h
        ld      CTX_COUNT+1(iy),a
        ret

__stdio_set_count_zero::
        xor     a
        ld      CTX_COUNT(iy),a
        ld      CTX_COUNT+1(iy),a
        ret

__stdio_emit_a::
        push    bc
        push    de
        push    hl
        ld      b,a
        call    __stdio_load_count_hl
        inc     hl
        call    __stdio_store_count_hl
        ld      a,CTX_SINK_KIND(iy)
        or      a
        jr      z,__stdio_emit_console
        cp      #SINK_STRING
        jr      z,__stdio_emit_string
        call    __stdio_load_sink_room_hl
        ld      a,h
        or      l
        jr      z,__stdio_emit_done
        call    __stdio_load_sink_ptr_hl
        ld      a,b
        ld      (hl),a
        inc     hl
        call    __stdio_store_sink_ptr_hl
        call    __stdio_load_sink_room_hl
        dec     hl
        call    __stdio_store_sink_room_hl
        jr      __stdio_emit_done
__stdio_emit_console:
        ;; Console streams go to the platform putchar hook (not write(),
        ;; which is for disk block I/O only).
        push    iy
        ld      l,b
        ld      h,#0x00
        call    _putchar
        pop     iy
        jr      __stdio_emit_done
__stdio_emit_string:
        call    __stdio_load_sink_ptr_hl
        ld      a,b
        ld      (hl),a
        inc     hl
        call    __stdio_store_sink_ptr_hl
__stdio_emit_done:
        pop     hl
        pop     de
        pop     bc
        ret

