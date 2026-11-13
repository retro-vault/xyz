        ; Allocate an image buffer from any logical bank.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _bank_allocate
        .optsdcc -mz80 sdcccall(1)

        .globl  __bank_allocate, __bank_allocate_library
        .globl  __bank_current
        .globl  __bank_count
        .globl  __bank_map
        .globl  __current_process
        .globl  _mem_allocate
        .globl  _enter_critical_section
        .globl  _leave_critical_section

        .area   _EXTRA

        ; input: HL = requested bytes
        ; output: DE = payload and A = bank; both zero on failure
        ; The selected bank remains mapped on success. Preserves IX/IY.
        ; Libraries get an empty bank, retaining near-allocation capacity
        ; in application banks even when a service image becomes smaller.
__bank_allocate_library::
        set 7,h
__bank_allocate::
        push    ix
        push    iy
        push    hl
        pop     iy
        call    _enter_critical_section
        call    __current_process
        push    bc
        pop     ix
        xor     a
.next:
        push    af
        call    __bank_map
        push    iy
        pop     de
        bit     7,d
        jr      nz,.library_bank
        jr .allocate
.library_bank:
        ld      hl,#0xc004
        bit     0,(hl)
        jr      nz,.skip
        ld      hl,(0xc000)
        ld      a,h
        or      l
        jr      nz,.skip
.allocate:
        push    ix
        push    iy
        pop     de
        res     7,d
        ld      hl,#0xc000
        call    _mem_allocate
        pop     af
        ld      c,a
        ld      a,d
        or      e
        ld      a,c
        jr      nz,.done
        jr      .advance
.skip:
        pop     af
.advance:
        inc     a
        ld      hl,#__bank_count
        cp      (hl)
        jr      c,.next
        xor     a
.done:
        call    _leave_critical_section
        pop     iy
        pop     ix
        ret
