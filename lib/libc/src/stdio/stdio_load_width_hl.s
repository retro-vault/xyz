        ;; stdio_load_width_hl.s
        ;; Split from printf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module stdio_load_width_hl
        .optsdcc -mz80 sdcccall(1)

        .globl  __stdio_emit_padding
        .globl  __stdio_emit_string_field
        .globl  __stdio_load_width_hl
        .globl  __stdio_reset_field_state
        .globl  __stdio_store_precision_hl
        .globl  __stdio_store_width_hl
        .globl  __stdio_emit_a

CTX_FLAGS       .equ 3
CTX_HAVE_PREC   .equ 5
CTX_LENGTH      .equ 4
CTX_PRECISION   .equ 22
CTX_PREFIX_LEN  .equ 9
CTX_SIGN        .equ 8
CTX_UPPER       .equ 7
CTX_WIDTH       .equ 20
LEN_WORD        .equ 0x02

        .area   _CODE
__stdio_load_width_hl::
        ld      a,CTX_WIDTH(iy)
        ld      l,a
        ld      a,CTX_WIDTH+1(iy)
        ld      h,a
        ret

__stdio_store_width_hl::
        ld      a,l
        ld      CTX_WIDTH(iy),a
        ld      a,h
        ld      CTX_WIDTH+1(iy),a
        ret

__stdio_store_precision_hl::
        ld      a,l
        ld      CTX_PRECISION(iy),a
        ld      a,h
        ld      CTX_PRECISION+1(iy),a
        ret

__stdio_reset_field_state::
        xor     a
        ld      CTX_FLAGS(iy),a
        ld      CTX_HAVE_PREC(iy),a
        ld      CTX_SIGN(iy),a
        ld      CTX_PREFIX_LEN(iy),a
        ld      CTX_UPPER(iy),a
        ld      a,#LEN_WORD
        ld      CTX_LENGTH(iy),a
        ld      hl,#0x0000
        call    __stdio_store_width_hl
        jp      __stdio_store_precision_hl

__stdio_emit_padding::
        push    af
        ld      a,b
        or      c
        jr      nz,__stdio_emit_padding_loop
        pop     af
        ret
__stdio_emit_padding_loop:
        pop     af
        push    af
        call    __stdio_emit_a
        dec     bc
        ld      a,b
        or      c
        jr      nz,__stdio_emit_padding_loop
        pop     af
        ret

__stdio_emit_string_field::
        push    hl                        ; preserve start pointer for emission
        ld      de,#0x0000
__stdio_emit_string_len_loop:
        ld      a,(hl)
        or      a
        jr      z,__stdio_emit_string_len_done
        ld      a,CTX_HAVE_PREC(iy)
        or      a
        jr      z,__stdio_emit_string_len_advance
        ld      a,d
        ld      b,a
        ld      a,CTX_PRECISION+1(iy)
        cp      b
        jr      nz,__stdio_emit_string_len_advance
        ld      a,e
        ld      b,a
        ld      a,CTX_PRECISION(iy)
        cp      b
        jr      z,__stdio_emit_string_len_done
__stdio_emit_string_len_advance:
        inc     hl
        inc     de
        jr      __stdio_emit_string_len_loop
__stdio_emit_string_len_done:
        call    __stdio_load_width_hl
        or      a
        sbc     hl,de
        jr      nc,__stdio_emit_string_pad_ok
        ld      hl,#0x0000
__stdio_emit_string_pad_ok:
        ld      b,h
        ld      c,l
        pop     hl
        ld      a,CTX_FLAGS(iy)
        bit     0,a
        jr      nz,__stdio_emit_string_body
        ld      a,#' '
        call    __stdio_emit_padding
__stdio_emit_string_body:
        ld      a,d
        or      e
        jr      z,__stdio_emit_string_tail
        push    bc
        ld      b,d
        ld      c,e
__stdio_emit_string_chars:
        ld      a,(hl)
        call    __stdio_emit_a
        inc     hl
        dec     bc
        ld      a,b
        or      c
        jr      nz,__stdio_emit_string_chars
        pop     bc
__stdio_emit_string_tail:
        ld      a,CTX_FLAGS(iy)
        bit     0,a
        ret     z
        ld      a,#' '
        jp      __stdio_emit_padding

