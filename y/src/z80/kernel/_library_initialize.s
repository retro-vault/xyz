        ; Initialize a library with owned, unpublished resources.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _library_initialize
        .optsdcc -mz80 sdcccall(1)
        .globl  __library_initialize
        .globl  _thread_current
        .globl  __library_private_services
        .globl  __process_find_owned
        .globl  __string_compare
        .globl  _svc_register
        .globl  _enter_critical_section
        .globl  _leave_critical_section
        .globl  __bank_call_iy
        .area   _CODE

        ; inputs: ix = loader frame, iy = library process
        ; outputs: de = staged service, or zero / a = error
        ; clobbers: af, bc, de, hl; preserves ix and iy
        ; Temporarily overrides thread.owner, never thread.process:
        ; the client's actual thread remains alive throughout init.
        ; Stack saves the override's address and previous far-owner value.
__library_initialize::
        call    _enter_critical_section
        ld      hl, (_thread_current)
        inc     hl
        inc     hl
        push    hl
        ld      a, (hl)
        push    af
        inc     hl
        ld      c, (hl)
        inc     hl
        ld      b, (hl)
        push    bc
        push    iy
        pop     de
        ld      a, 16(iy)
        ld      (hl), d
        dec     hl
        ld      (hl), e
        dec     hl
        ld      (hl), a
        call    _leave_critical_section
        bit     1, 7(ix)
        jr      z, .register
        ld      l, 74(ix)
        ld      h, 75(ix)
        call    .initialize
        ld      a, d
        or      e
        ld      a, #11
        jr      nz, .failed
.register:
        call    _enter_critical_section
        ld      hl, (__library_private_services)
        push    iy
        pop     de
        call    __process_find_owned
        ld      a, d
        or      e
        jr      nz, .validate
        ld      e, 74(ix)
        ld      d, 75(ix)
        call    .name
        call    _svc_register
        ld      a, d
        or      e
        ld      a, #2
        jr      z, .restore
.validate:
        push    de
        ld      hl, #5
        add     hl, de
        ex      de, hl
        call    .name
        call    __string_compare
        ld      a, d
        or      e
        pop     de
        ld      a, #4
        jr      nz, .invalid
        ld      hl, #21
        add     hl, de
        ld      a, 74(ix)
        ld      (hl), a
        inc     hl
        ld      a, 75(ix)
        ld      (hl), a
        xor     a
        jr      .restore
.invalid:
        ld      de, #0
        jr      .restore
.failed:
        ld      de, #0
        call    _enter_critical_section
.restore:
        ex      af, af'                ; preserve init/validation status
        pop     bc
        pop     af
        pop     hl
        ld      (hl), a
        inc     hl
        ld      (hl), c
        inc     hl
        ld      (hl), b
        ex      af, af'
        jp      _leave_critical_section
.name:
        push    ix
        pop     hl
        ld      bc, #40
        add     hl, bc
        ret
        ; Invoke the relocated initializer, retaining its HL argument.
.initialize:
        push    iy                      ; keep the library object live in IY
        push    hl
        ld      l, 76(ix)
        ld      h, 77(ix)
        push    hl
        pop     iy
        xor     a
        ld      d, a
        ld      e, a
        ex      af,af'                  ; initializer A argument is zero
        ld      a, 82(ix)
        pop     hl                      ; common far-interface argument
        exx                            ; protect initializer registers
        ld      de, #.initialized
        jp      __bank_call_iy
.initialized:
        pop     iy
        ret
