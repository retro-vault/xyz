        ;; string_char_in_set.s
        ;; Split from string_common.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module string_char_in_set
        .optsdcc -mz80 sdcccall(1)

        .globl  __string_char_in_set

        .area   _CODE
__string_char_in_set::
        push    bc
        ld      b,a
__string_char_in_set_loop:
        ld      a,(hl)
        or      a
        jr      z,__string_char_in_set_not_found
        cp      b
        jr      z,__string_char_in_set_found
        inc     hl
        jr      __string_char_in_set_loop
__string_char_in_set_found:
        pop     bc
        ret
__string_char_in_set_not_found:
        pop     bc
        inc     a
        ret

        ; __string_fold_lower
        ; inputs:  A = ASCII byte
        ; outputs: A = tolower(byte) for 'A'..'Z', unchanged otherwise
        ; clobbers: F
        ; preserves: BC, DE, HL
