        ; strsep.s
        ;
        ; libc strsep implementation for the xcc Z80 libc (BSD extension).
        ; Finds the next token in *stringp delimited by any byte in delim:
        ; the delimiter is overwritten with NUL, *stringp is advanced past it
        ; (or set to NULL at the end), and the original token is returned.
        ; A NULL *stringp yields NULL.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module strsep
        .optsdcc -mz80 sdcccall(1)


        .globl  _strsep
        .globl  __string_char_in_set

        .area   _CODE

        ; _strsep
        ; inputs:  HL = char **stringp, DE = const char *delim
        ; outputs: DE = token pointer (or 0)
        ; clobbers: AF, BC, HL
_strsep::
        push    hl                      ; [A] save stringp
        ; tok = *stringp
        ld      a,(hl)
        inc     hl
        ld      h,(hl)
        ld      l,a                     ; HL = tok
        ld      a,h
        or      l
        jr      z,strsep_null           ; *stringp == NULL
        push    hl                      ; [B] save tok (return base)
        ld      b,d
        ld      c,e                     ; BC = delim base (survives the helper)
        ex      de,hl                   ; DE = p = tok
strsep_scan:
        ld      a,(de)
        or      a
        jr      z,strsep_nodelim        ; end of string, no delimiter
        ld      h,b
        ld      l,c                     ; HL = delim set
        call    __string_char_in_set    ; Z = char is a delimiter (keeps BC,DE)
        jr      z,strsep_found
        inc     de
        jr      strsep_scan
strsep_found:
        xor     a
        ld      (de),a                  ; terminate the token
        inc     de                      ; DE = p + 1 = new *stringp
        pop     bc                      ; [B] BC = tok
        pop     hl                      ; [A] HL = stringp
        ld      (hl),e
        inc     hl
        ld      (hl),d                  ; *stringp = p + 1
        ld      d,b
        ld      e,c                     ; DE = tok
        ret
strsep_nodelim:
        pop     bc                      ; [B] BC = tok
        pop     hl                      ; [A] HL = stringp
        xor     a
        ld      (hl),a
        inc     hl
        ld      (hl),a                  ; *stringp = NULL
        ld      d,b
        ld      e,c                     ; DE = tok
        ret
strsep_null:
        pop     hl                      ; discard [A]
        ld      de,#0
        ret
