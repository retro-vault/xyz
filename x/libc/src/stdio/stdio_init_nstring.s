        ;; stdio_init_nstring.s
        ;; Split from printf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module stdio_init_nstring
        .optsdcc -mz80 sdcccall(1)

        .globl  __stdio_init_nstring
        .globl  __stdio_set_count_zero
        .globl  __stdio_store_sink_ptr_hl
        .globl  __stdio_store_sink_room_hl

CTX_SINK_KIND   .equ 0
CTX_SINK_TERM   .equ 1
SINK_NSTRING    .equ 0x02

        .area   _CODE
__stdio_init_nstring::
        ld      a,#SINK_NSTRING
        ld      CTX_SINK_KIND(iy),a
        xor     a
        ld      CTX_SINK_TERM(iy),a
        call    __stdio_store_sink_ptr_hl
        ld      a,d
        or      e
        jr      z,__stdio_init_nstring_room_zero
        ld      a,#1
        ld      CTX_SINK_TERM(iy),a
        dec     de
        push    de
        pop     hl
        call    __stdio_store_sink_room_hl
        jp      __stdio_set_count_zero
__stdio_init_nstring_room_zero:
        push    de
        pop     hl
        call    __stdio_store_sink_room_hl
        jp      __stdio_set_count_zero

