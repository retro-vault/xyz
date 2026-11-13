        ; Next RAM kernel reset and restart-vector header.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module crt0rom
        .optsdcc -mz80 sdcccall(1)

        .globl  _main
        .globl  __startup_init
        .globl  __sys_vec_tbl
        .globl  __sys_stack
        .globl  __console_print

        .area   _HEADER

.org    0x0000
.reset:                                ; RAM kernel entry
        di
        xor     a
        jp      .init
        .db     0, 0, 0

        ; Filesystem calls use the native ROM jump table, not RST 08h.
.rst08:
        ret
        .ds 7

        ; YOS restart vectors dispatch through the writable RAM jump table.
.rst10:                                ; console character output, A = byte
        push    hl
        call    __console_print              ; guarded until RAM initialization
        pop     hl
        ret
        .db     0, 0

        ; The remaining restart vectors dispatch through writable RAM.
.rst18:                                ; named-service lookup for RAM processes
        jp      __sys_vec_tbl + 6
        .db     0, 0, 0, 0, 0
.rst20:                                ; installable YOS restart handler
        jp      __sys_vec_tbl + 9
        .db     0, 0, 0, 0, 0
.rst28:                                ; installable YOS restart handler
        jp      __sys_vec_tbl + 12
        .db     0, 0, 0, 0, 0
.rst30:                                ; installable YOS restart handler
        jp      __sys_vec_tbl + 15
        .db     0, 0, 0, 0, 0

        ; Default IM1 interrupt return; YOS schedules processes in IM2.
.rst38:                                ; default IM1 interrupt return
        push    af
        pop     af
        ei
        reti
        .db     0, 0, 0, 0, 0, 0, 0, 0
        .db     0, 0, 0, 0, 0, 0, 0, 0
        .db     0, 0, 0, 0, 0, 0, 0, 0
        .db     0, 0, 0, 0, 0, 0, 0, 0
        .db     0, 0, 0, 0, 0, 0, 0, 0
        .db     0

.nmi:
        jp __sys_vec_tbl+21
        .ds 18

        ; Reserved firmware-independent header bytes.
.native_byte_read:
        ld      a, (hl)
        ret

        ; The RAM kernel starts at the conventional 0100h
        ; base-ROM entry. The unused 0080h..00ffh half holds immutable YOS
        ; tables; execution still begins at the conventional 0100h address.
        .ds     3

        .area   _CODE
.init:
        ld      sp, #0xffff
        call    __startup_init
        ld      sp, #__sys_stack
        im      1
        di                              ; main installs scheduler before EI
        call    _main
.tarpit:
        halt
        jr      .tarpit

        .area   _INITIALIZER
        .area   _INITFINAL
        .area   _GSINIT
        .area   _GSFINAL
        .area   _CONST
        .area   _DATA
        .area   _INITIALIZED
        .area   _BSS
        .area   _HEAP
