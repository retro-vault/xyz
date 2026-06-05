        ; string_common.s
        ;
        ; Shared helper routines for the libc string implementation.
        ; The public entry points stay small by routing common return-value,
        ; scanning, copying, and set-membership logic through this file.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module string_common
        .optsdcc -mz80 sdcccall(1)


        .globl  __string_return_zero
        .globl  __string_return_hl
        .globl  __string_return_bc
        .globl  __string_compare_result
        .globl  __string_scan_nul
        .globl  __string_copy_cstr
        .globl  __string_char_in_set

        .area   _CODE

        ; __string_return_zero
        ; outputs: DE = 0
__string_return_zero::
        ld      de,#0x0000
        ret

        ; __string_return_hl
        ; inputs: HL = pointer / size / scalar result
        ; outputs: DE = HL
__string_return_hl::
        ex      de,hl
        ret

        ; __string_return_bc
        ; inputs: BC = pointer / size result
        ; outputs: DE = BC
__string_return_bc::
        ld      d,b
        ld      e,c
        ret

        ; __string_compare_result
        ; inputs: flags from a prior CP/compare
        ; outputs: DE = -1, 0, or 1
        ; notes:
        ;   The helper assumes the caller preserved the compare flags and wants
        ;   a normal C strcmp-style tri-state result.
__string_compare_result::
        ld      de,#0xffff
        jr      c,__string_compare_result_done
        ld      de,#0x0000
        ret     z
        inc     e
__string_compare_result_done:
        ret

        ; __string_scan_nul
        ; inputs: HL = start of a NUL-terminated string
        ; outputs: HL = address of the terminating NUL byte
        ; clobbers: AF, BC
        ; notes:
        ;   CPIR stops one byte past the match, so HL is adjusted back.
__string_scan_nul::
        xor     a
        ld      bc,#0xffff
        cpir
        dec     hl
        ret

        ; __string_copy_cstr
        ; inputs: HL = source string, DE = destination string
        ; outputs: HL/DE advanced just past the copied NUL terminator
        ; clobbers: AF
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
