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

        .area   _CODE

        ; _memrchr
        ; inputs: HL = span, DE = search byte (E), 2(sp)..3(sp) = count
        ; stack argument remains for the caller to remove
        ; outputs: DE = pointer to the last matching byte, or 0
        ; clobbers: AF, BC, HL; preserves IX and IY
_memrchr::
        pop     bc                      ; return address
        pop     af                      ; raw byte count
        push    af                      ; retain caller's argument
        push    bc
        push    af
        pop     bc
        ld      a,b
        or      c
        jr      z,memrchr_not_found
        add     hl,bc
        dec     hl
        ld      a,e
        cpdr                            ; stop on equality or count exhaustion
        jr      nz,memrchr_not_found
        inc     hl                      ; CPDR stepped past the matching byte
        ex      de,hl
        ret
memrchr_not_found:
        ld      de,#0
        ret
