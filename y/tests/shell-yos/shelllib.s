        ; Real XL library: relocated functions, data and self-registration.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module shelllib
        .optsdcc -mz80 sdcccall(1)
        .include "../../include/yos.inc"
        .globl  _entry
        .globl  _probe
        .globl  _message
        .globl  _initializations
        .globl  _calls
        .globl  _sum3
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
        ld      bc, #YOS_OFFSET_ALLOCATE_MEMORY
        add     hl, bc
        call    .function
        ld      hl, #16
        call    .invoke
        ld      (_storage), hl
        ld      a,e
        ld      (.storage_bank),a
        or      h
        or      l
        jr      z, .failed
        ld      a,(.storage_bank)
        ld      c,a
        ld      de,#.message
.copy_message:
        ld      a,(de)
        scf
        rst     0x30                    ; write A to C:HL
        inc     de
        inc     hl
        or      a
        jr      nz,.copy_message
        ld      hl, (.yos_table)
        ld      bc, #YOS_OFFSET_REGISTER_SERVICE
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
        ld      hl, (_storage)
        ld      a,(.storage_bank)
        ld      e,a
        ld      d,#0
        ret
_initializations::
        ld      de, (.init_count)
        ret
_calls::
        ld      de, (.call_count)
        ret
        ; Three-word sdcccall(1): HL + DE + one callee-cleaned stack word.
_sum3::
        push    ix
        ld      ix, #0
        add     ix, sp
        ld      c, 4(ix)
        ld      b, 5(ix)
        add     hl, de
        add     hl, bc
        ex      de, hl
        pop     ix
        pop     hl                      ; return address
        pop     bc                      ; remove third argument
        push    hl
        ret

        .area   _DATA
_interface::
        .dw     _probe, _message, _initializations, _calls, _sum3
.yos_table:
        .dw     0
_storage::
        .dw     0
.storage_bank:
        .db     0
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
