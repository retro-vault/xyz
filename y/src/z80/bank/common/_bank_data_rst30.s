        ; Common-memory byte access for XCC far data pointers.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _bank_data_rst30
        .optsdcc -mz80 sdcccall(1)

        .globl  __bank_data_rst30
        .globl  __bank_current
        .globl  __bank_map
        .globl  _enter_critical_section
        .globl  _leave_critical_section

        .area   _CODE

        ; RST 30h far-byte gate used by x/platforms/yos/far.s.
        ; inputs: HL = address, C = logical bank
        ;         carry clear: read byte; carry set: write byte from A
        ; output: A = loaded/stored byte
        ; Preserves BC/DE/HL/IX/IY and restores the execution bank.
__bank_data_rst30::
        push    bc
        push    de
        push    hl
        ld      e,a                     ; write value / eventual read result
        ld      d,#0
        rl      d                       ; D = 1 for write, zero for read
        call    _enter_critical_section
        ld      a,(__bank_current)
        push    af
        ld      a,c
        call    __bank_map
        ld      a,d
        or      a
        ld      a,e
        jr      nz,.put
        ld      a,(hl)
        ld      e,a
        jr      .restore
.put:
        ld      (hl),a
.restore:
        pop     af
        call    __bank_map
        call    _leave_critical_section
        ld      a,e
        pop     hl
        pop     de
        pop     bc
        ret
