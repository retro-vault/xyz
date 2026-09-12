        ; Bind exports, initialize and commit a threadless library.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _library_load_finish
        .optsdcc -mz80 sdcccall(1)
        .globl  __library_load_finish
        .globl  __library_acquire
        .globl  __image_transfer
        .globl  __library_initialize
        .globl  _process_first
        .globl  __svc_first
        .globl  __library_private_services
        .globl  _so_create
        .globl  _list_remove
        .globl  _list_insert
        .globl  _process_reap
        .globl  _enter_critical_section
        .globl  _leave_critical_section
        .equ    LIBRARY_SIZE,   15
        .equ    LIBRARY_FLAGS,   4
        .equ    LIBRARY_SERVICE, 5
        .equ    LIBRARY_ABI,     7
        .equ    LIBRARY_REFS,   13
        .area   _CODE

        ; inputs: ix = loader frame; bc = XL code size
        ; outputs: de = interface or zero, a = load error on failure
        ; clobbers: af, bc, de, hl; preserves ix and iy
        ; No thread/stack is created. The optional initializer receives
        ; HL = interface, DE = zero on success; IX/IY preserved.
        ; Init allocations and registrations belong to the library.
__library_load_finish::
        push    iy
        ld      12(ix), c
        ld      13(ix), b
        ld      e, 66(ix)
        ld      d, 67(ix)
        push    de
        pop     iy
        ld      b, 34(ix)
.export:
        ld      a, (de)
        cp      #0xc3
        jr      nz, .invalid
        inc     de
        ld      a, (de)
        ld      l, a
        inc     de
        ld      a, (de)
        ld      h, a
        inc     de
        push    de
        ld      e, 12(ix)
        ld      d, 13(ix)
        or      a
        sbc     hl, de
        pop     de
        jr      nc, .invalid
        push    de
        ld      e, 12(ix)
        ld      d, 13(ix)
        add     hl, de
        ld      e, 68(ix)
        ld      d, 69(ix)
        add     hl, de
        pop     de
        ld      0(iy), l
        ld      1(iy), h
        inc     iy
        inc     iy
        djnz    .export
        jr      .create
.invalid:
        ld      a, #4
.failed:
        ld      de, #0
        pop     iy
        ret
.create:
        call    _enter_critical_section
        ld      hl, #0
        push    hl
        ld      hl, #_process_first
        ld      de, #LIBRARY_SIZE
        call    _so_create
        ld      a, d
        or      e
        jr      z, .no_object
        push    de
        pop     iy
        ld      a, 70(ix)
        ld      LIBRARY_FLAGS(iy), a
        ld      a, 6(ix)
        ld      LIBRARY_ABI(iy), a
        xor     a
        ld      LIBRARY_REFS(iy), a
        ld      LIBRARY_REFS+1(iy), a
        ld      l, 66(ix)
        ld      h, 67(ix)
        ld      74(ix), l              ; retain table across transfer
        ld      75(ix), h
        call    __image_transfer
        call    _leave_critical_section
        call    __library_initialize
        call    _enter_critical_section
        ld      b, a
        ld      a, d
        or      e
        ld      a, b
        jr      z, .rollback
        ld      LIBRARY_SERVICE(iy), e
        ld      LIBRARY_SERVICE+1(iy), d
        bit     1, LIBRARY_FLAGS(iy)
        jr      z, .acquire
        ld      hl, #__library_private_services
        call    _list_remove
        ld      hl, #__svc_first
        call    _list_insert
.acquire:
        push    iy
        pop     hl
        ld      e, 78(ix)
        ld      d, 79(ix)
        call    __library_acquire
        ld      b, a
        ld      a, d
        or      e
        ld      a, b
        jr      nz, .done
.rollback:
        push    af
        push    iy
        pop     hl
        call    _process_reap
        pop     af
        ld      de, #0
        jr      .done
.no_object:
        ld      de, #0
        ld      a, #2
.done:
        call    _leave_critical_section
        pop     iy
        ret
