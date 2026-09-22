        ; Change the state of a registered YOS event.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module evt_set
        .optsdcc -mz80 sdcccall(1)
        .globl  _evt_set
        .globl  _enter_critical_section
        .globl  _leave_critical_section
        .globl  __evt_first
        .globl  __frame_ix

        .area   _CODE

        ; hl = event, new state byte at sp+2; removes state argument.
_evt_set::
        call    _enter_critical_section
        call    __frame_ix
        ex      de, hl
        ld      hl, (__evt_first)
        ld      b, #0
.loop:
        ld      a, h
        or      l
        jr      z, .absent
        ld      a, h
        cp      d
        jr      nz, .next
        ld      a, l
        cp      e
        jr      z, .found
.next:
        ld      a, (hl)
        inc     hl
        ld      h, (hl)
        ld      l, a
        djnz    .loop
.absent:
        ld      de, #0
        jr      .done
.found:
        inc     hl
        inc     hl
        inc     hl
        inc     hl
        ld      a, 4(ix)
        ld      (hl), a
.done:
        call    _leave_critical_section
        pop     ix
        pop     hl
        inc     sp
        jp      (hl)
