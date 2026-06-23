        ; strcasestr.s
        ;
        ; libc strcasestr implementation for the xcc Z80 libc (GNU extension).
        ; Case-insensitive strstr: nested scan with both bytes folded to lower
        ; case before comparison.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module strcasestr
        .optsdcc -mz80 sdcccall(1)


        .globl  _strcasestr
        .globl  __string_fold_lower
        .globl  __string_return_zero
        .globl  __string_return_hl

        .area   _CODE

        ; _strcasestr
        ; inputs:  HL = haystack, DE = needle
        ; outputs: DE = pointer to the first match, or 0
        ; clobbers: AF, BC, HL
_strcasestr::
        ld      a,(de)
        or      a                       ; empty needle matches at the start
        jp      z,__string_return_hl
scs_outer:
        ld      a,(hl)
        or      a
        jr      z,scs_not_found
        push    hl                      ; save haystack candidate
        push    de                      ; save needle base
scs_inner:
        ld      a,(de)
        or      a
        jr      z,scs_found
        call    __string_fold_lower     ; A = tolower(needle char); keeps BC,DE,HL
        ld      c,a
        ld      a,(hl)
        or      a
        jr      z,scs_mismatch
        call    __string_fold_lower     ; A = tolower(haystack char)
        cp      c
        jr      nz,scs_mismatch
        inc     hl
        inc     de
        jr      scs_inner
scs_found:
        pop     de                      ; drop needle base
        pop     de                      ; DE = candidate = match
        ret
scs_mismatch:
        pop     de
        pop     hl
        inc     hl
        jr      scs_outer
scs_not_found:
        jp      __string_return_zero
