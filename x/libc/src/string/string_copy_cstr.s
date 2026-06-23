        ;; string_copy_cstr.s
        ;; Split from string_common.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module string_copy_cstr
        .optsdcc -mz80 sdcccall(1)

        .globl  __string_copy_cstr

        .area   _CODE
__string_copy_cstr::
__string_copy_cstr_loop:
        ld      a,(hl)
        ld      (de),a
        inc     hl
        inc     de
        or      a
        jr      nz,__string_copy_cstr_loop
        ret

        ; __string_char_in_set
        ; inputs:
        ;   A  = character to test
        ;   HL = NUL-terminated character-set string
        ; outputs:
        ;   Z  = 1 if the character was found
        ;   Z  = 0 if the character was not found
        ; clobbers: AF, HL
        ; preserves: BC
