        ; Bind exports, initialize and commit a threadless library.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _library_load_finish
        .optsdcc -mz80 sdcccall(1)
        .globl  __library_load_finish
        .globl  __library_acquire
        .globl  __image_retain
        .globl  __image_transfer
        .globl  __os_malloc
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
        .equ    LIBRARY_SIZE,   17
        .equ    LIBRARY_FLAGS,   5
        .equ    LIBRARY_SERVICE, 6
        .equ    LIBRARY_ABI,     8
        .equ    LIBRARY_REFS,   14
        .equ    IMAGE_BANK,     82
        .equ    IMAGE_TABLE,    84
        .area   _CODE

        ; inputs: ix = loader frame, +12 holds the relocated code end
        ; outputs: de = interface or zero, a = load error on failure
        ; clobbers: af, bc, de, hl; preserves ix and iy
        ; No thread/stack is created. The optional initializer receives
        ; HL = interface, DE = zero on success; IX/IY preserved.
        ; Init allocations and registrations belong to the library.
__library_load_finish::
        push    iy
        ld      l, 72(ix)              ; three bytes per export
        ld      h, 73(ix)
        call    __os_malloc
        ld      a, d
        or      e
        ld      a, #2
        jr      z, .failed
        ld      IMAGE_TABLE(ix), e
        ld      IMAGE_TABLE+1(ix), d
        ld      l, 80(ix)              ; raw JP metadata is temporary
        ld      h, 81(ix)
        ld      b, 34(ix)
.export:
        ld      a, (hl)
        cp      #0xc3
        jr      nz, .invalid
        inc     hl
        push    bc                      ; export count
        ld      c, (hl)
        inc     hl
        ld      b, (hl)
        inc     hl
        push    hl                      ; next raw JP
        ld      l, 68(ix)
        ld      h, 69(ix)
        add     hl, bc                 ; absolute export target
        jr      c, .invalid_target
        ld      c, 12(ix)
        ld      b, 13(ix)
        or      a
        sbc     hl, bc
        jr      nc, .invalid_target    ; at or past the relocated code end
        add     hl, bc
        ex      de, hl                  ; DE target, HL common table cursor
        ld      a, IMAGE_BANK(ix)
        ld      (hl), a
        inc     hl
        ld      (hl), e
        inc     hl
        ld      (hl), d
        inc     hl
        ex      de, hl
        pop     hl
        pop     bc
        djnz    .export
        jr      .create
.invalid_target:
        pop     hl
        pop     bc
.invalid:
        ld      a, #4
.failed:
        ld      de, #0
        pop     iy
        ret
.create:
        call    __image_retain
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
        call    __image_transfer
        ; Transfer the common far table to the library owner too.
        ld      l, IMAGE_TABLE(ix)
        ld      h, IMAGE_TABLE+1(ix)
        ld      74(ix), l
        ld      75(ix), h
        ld      bc, #-5                 ; heap-block owner remains a near ID
        add     hl, bc
        ld      (hl), e
        inc     hl
        ld      (hl), d
        xor     a
        ld      IMAGE_TABLE(ix), a
        ld      IMAGE_TABLE+1(ix), a
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
        jr      .zero_result
.no_object:
        ld      a, #2
.zero_result:
        ld      de, #0
.done:
        call    _leave_critical_section
        pop     iy
        ret
