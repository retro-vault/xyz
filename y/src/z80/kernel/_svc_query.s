        ; Find a named service while protecting the registration list.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module _svc_query
        .optsdcc -mz80 sdcccall(1)
        .globl  __svc_query
        .globl  __svc_first
        .globl  __string_compare
        .globl  _enter_critical_section
        .globl  _leave_critical_section
        .equ    SERVICE_NAME,       4
        .equ    SERVICE_INTERFACE, 20
        .area   _CODE

        ; inputs: hl = name; outputs: de = borrowed table or zero
        ; clobbers: af, bc, de, hl; preserves ix and iy
        ; IX keeps the name; the examined service is saved on stack.
__svc_query::
        call    _enter_critical_section
        push    ix
        push    hl
        pop     ix
        ld      hl, (__svc_first)
        ld      b, #0
.loop:
        ld      a, h
        or      l
        jr      z, .missing
        push    hl
        ld      de, #SERVICE_NAME
        add     hl, de
        ex      de, hl
        push    ix
        pop     hl
        push    bc
        call    __string_compare
        pop     bc
        pop     hl
        ld      a, d
        or      e
        jr      z, .found
        ld      a, (hl)
        inc     hl
        ld      h, (hl)
        ld      l, a
        djnz    .loop
.missing:
        ld      de, #0
        jr      .done
.found:
        ld      de, #SERVICE_INTERFACE
        add     hl, de
        ld      e, (hl)
        inc     hl
        ld      d, (hl)
.done:
        pop     ix
        jp      _leave_critical_section
