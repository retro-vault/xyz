        ; Real XL library: relocated functions, data and self-registration.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module shelllib
        .optsdcc -mz80 sdcccall(1)
        .globl  _entry
        .globl  _probe
        .globl  _message
        .globl  _initializations
        .globl  _calls
        .globl  _interface
        .globl  _init_status
        .globl  _storage
        .globl  _name
        .globl  _registered
        .area   _CODE

        ; HL = loader's bound export table (unused: publish our own).
        ; DE = zero on success; preserves IX/IY. No process CRT or exit.
_entry::
        push    iy
        ld      hl, #.init_count
        inc     (hl)
        ld      hl, #.yos_name
        rst     0x18
        ld      (.yos_table), de
        ex      de, hl
        inc     hl
        inc     hl
        call    .function
        ld      hl, #16
        call    .invoke
        ld      (_storage), de
        ld      a, d
        or      e
        jr      z, .failed
        ld      hl, (.yos_table)
        ld      bc, #36                ; yos_t::register_service
        add     hl, bc
        call    .function
        ld      hl, #_name
        ld      de, #_interface
        call    .invoke
_registered::
        ld      a, d
        or      e
        jr      z, .failed
        ld      de, (_init_status)
        pop     iy
        ret
.failed:
        ld      de, #1
        pop     iy
        ret
.function:
        ld      e, (hl)
        inc     hl
        ld      d, (hl)
        push    de
        pop     iy
        ret
.invoke:
        jp      (iy)

        ; No arguments; return a signature and count the actual calls.
_probe::
        ld      hl, #.call_count
        inc     (hl)
        ld      de, #0x600d
        ret
_message::
        ld      de, #.message
        ret
_initializations::
        ld      de, (.init_count)
        ret
_calls::
        ld      de, (.call_count)
        ret

        .area   _DATA
_interface::
        .dw     _probe, _message, _initializations, _calls
.yos_table:
        .dw     0
_storage::
        .dw     0
.init_count:
        .dw     0
.call_count:
        .dw     0
_init_status::
        .dw     0

        .area   _CONST
_name::
        .asciz  "shelllib"
.yos_name:
        .asciz  "yos"
.message:
        .asciz  "Library OK"
