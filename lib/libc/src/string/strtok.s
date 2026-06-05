        ; strtok.s
        ;
        ; libc strtok implementation for the xcc Z80 libc.
        ; Maintains the continuation pointer in a small static cell and tokenizes
        ; the string in place by overwriting delimiters with NUL bytes.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module strtok
        .optsdcc -mz80 sdcccall(1)


        .globl  _strtok
        .globl  __string_char_in_set
        .globl  __string_return_zero

        .area   _CODE

        ; _strtok
        ; inputs:
        ;   HL = input string, or 0 to continue the previous tokenization
        ;   DE = delimiter-set string
        ; outputs:
        ;   DE = pointer to the next token, or 0 when no tokens remain
        ; clobbers: AF, BC, HL
_strtok::
        ld      a,h
        or      l
        jr      nz,strtok_have_input
        ld      hl,(__strtok_save)      ; resume from the prior call
        ld      a,h
        or      l
        jp      z,__string_return_zero
strtok_have_input:
        ld      b,d                      ; BC = delimiter-set pointer
        ld      c,e
strtok_skip_delims:
        ld      a,(hl)
        or      a
        jr      z,strtok_no_token
        push    hl                      ; preserve current scan position
        ld      h,b
        ld      l,c
        call    __string_char_in_set
        pop     hl
        jr      nz,strtok_token_start
        inc     hl
        jr      strtok_skip_delims
strtok_token_start:
        push    hl                      ; save token start for the return path
strtok_scan_token:
        ld      a,(hl)
        or      a
        jr      z,strtok_end_at_nul
        push    hl
        ld      h,b
        ld      l,c
        call    __string_char_in_set
        pop     hl
        jr      z,strtok_delim_hit
        inc     hl
        jr      strtok_scan_token
strtok_delim_hit:
        xor     a
        ld      (hl),a
        inc     hl
        ld      (__strtok_save),hl
        pop     de
        ret
strtok_end_at_nul:
        xor     a
        ld      (__strtok_save),a
        ld      (__strtok_save+1),a
        pop     de
        ret
strtok_no_token:
        xor     a
        ld      (__strtok_save),a
        ld      (__strtok_save+1),a
        jp      __string_return_zero

        .area   _BSS
__strtok_save:
        .ds     2
