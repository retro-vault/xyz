        ; Minimal assembly-only YOS kernel initialization.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module main
        .optsdcc -mz80 sdcccall(1)

        .globl  _main
        .globl  __sys_heap
        .globl  __heap
        .globl  __yos
        .globl  _mem_init
        .globl  _tmr_install
        .globl  _svc_register
        .globl  _svc_query_rst18
        .globl  __clock_tick
        .globl  __kbd_scan
        .globl  __mouse_scan
        .globl  __thread_robin
        .globl  __im2_init
        .globl  _sys_vec_set
        .globl  __yos_name
        .globl  __gpx_name
        .globl  __gpx_service
        ; Link roots fill otherwise unusable space before the 0562 trap.
        .globl  __critical_iff_repair
        .globl  _enter_critical_section
        .globl  __yos_malloc
        ; Keep the library initializer in the pre-loader packing pass.
        .globl  __library_initialize
        .globl  _boot_shell

        .area   _CODE

        ; Initialize heaps, clock, YOS service and scheduler vector.
_main::
        ld      de, #1024
        ld      hl, #__sys_heap
        call    _mem_init
        ld      hl, #0xffff
        ld      de, #__heap
        or      a
        sbc     hl, de
        ex      de, hl
        ld      hl, #__heap
        call    _mem_init

        ld      hl, #__clock_tick
        call    .install_tick_timer

        ld      hl, #__kbd_scan
        call    .install_tick_timer

        ld      hl, #__mouse_scan
        call    .install_tick_timer

        ld      de, #__yos
        ld      hl, #__yos_name
        call    _svc_register

        ld      de, #__gpx_service      ; publish complete graphics API
        ld      hl, #__gpx_name
        call    _svc_register

        ; Load the disk-resident user environment before enabling preemption.
        call    _boot_shell

        ld      a, #2                   ; RST 18h service query
        push    af
        inc     sp
        ld      hl, #_svc_query_rst18
        call    _sys_vec_set

        ; Arm IM2 preemption after the initial process has entered the queue.
        call    __im2_init

        ei                              ; loader and vectors are now complete
.idle:
        halt
        jr      .idle

        ; Install one kernel-owned callback on every frame tick.
.install_tick_timer:
        xor     a
        ld      d, a
        ld      e, a
        push    de
        call    _tmr_install
        ret
