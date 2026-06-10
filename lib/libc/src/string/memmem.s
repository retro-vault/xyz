        ;; memmem.s
        ;;
        ;; GNU memmem() for the xcc Z80 libc.
        ;; This is the byte-sequence analogue of strstr(): the haystack and
        ;; needle are bounded by explicit lengths, so embedded NUL bytes are
        ;; treated like ordinary data. The implementation keeps the current
        ;; haystack candidate in HL, the remaining candidate count in DE, and
        ;; reloads the needle base/length from the stack for each outer probe.

        .module memmem
        .optsdcc -mz80 sdcccall(1)

        .globl  _memmem
        .globl  __string_return_zero
        .globl  __string_return_hl

        .area   _CODE

_memmem::
        push    ix
        ld      ix,#0
        add     ix,sp

        ;; Empty needles match at the start of the haystack.
        ld      c,6(ix)                 ; BC = needle length
        ld      b,7(ix)
        ld      a,b
        or      c
        jr      nz,__memmem_nonempty
        pop     ix
        jp      __string_return_hl

__memmem_nonempty:
        ;; Compute how many candidate starts remain: hlen - nlen + 1.
        push    hl                      ; preserve haystack base
        ex      de,hl                   ; HL = haystack length
        xor     a
        sbc     hl,bc
        jr      c,__memmem_too_short
        inc     hl
        ex      de,hl                   ; DE = candidate count
        pop     hl                      ; HL = current haystack candidate

__memmem_outer:
        ld      a,d
        or      e
        jr      z,__memmem_not_found

        push    hl                      ; save candidate for mismatch/return
        push    de                      ; save remaining candidate count

        ld      c,4(ix)                 ; BC = needle base
        ld      b,5(ix)
        ld      e,6(ix)                 ; DE = remaining needle length
        ld      d,7(ix)

__memmem_inner:
        ld      a,d
        or      e
        jr      z,__memmem_found
        ld      a,(bc)
        cp      (hl)
        jr      nz,__memmem_mismatch
        inc     bc
        inc     hl
        dec     de
        jr      __memmem_inner

__memmem_found:
        pop     bc                      ; discard saved candidate count
        pop     de                      ; DE = matching candidate
        pop     ix
        ret

__memmem_mismatch:
        pop     de                      ; restore candidate count
        pop     hl                      ; restore candidate pointer
        inc     hl
        dec     de
        jr      __memmem_outer

__memmem_too_short:
        pop     hl                      ; drop saved haystack base
__memmem_not_found:
        pop     ix
        jp      __string_return_zero
