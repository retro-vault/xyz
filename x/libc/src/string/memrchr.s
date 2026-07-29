        ; memrchr.s
        ;
        ; libc memrchr implementation for the xcc Z80 libc.
        ; Scans a byte span from the end toward the start and returns a pointer
        ; to the last matching byte, or NULL (GNU extension).
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module memrchr
        .optsdcc -mz80 sdcccall(1)


        .globl  _memrchr
        .globl  __string_return_zero
        .globl  __string_return_hl

        .area   _CODE

        ; _memrchr
        ; inputs:  HL = start of span, DE = search byte (E), 4(ix)..5(ix) = count
        ; outputs: DE = pointer to the last matching byte, or 0
        ; clobbers: AF, BC, HL, IX
_memrchr::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,4(ix)
        ld      b,5(ix)
        ld      a,b
        or      c
        jr      z,memrchr_not_found
        ; advance HL to one past the last byte: HL += count
        add     hl,bc
memrchr_loop:
        dec     hl
        ld      a,(hl)
        cp      e
        jr      z,memrchr_found
        dec     bc
        ld      a,b
        or      c
        jr      nz,memrchr_loop
memrchr_not_found:
        pop     ix
        jp      __string_return_zero
memrchr_found:
        pop     ix
        jp      __string_return_hl
