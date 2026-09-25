        ; Return the read-only system-introspection descriptor.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module yos_get_sys_info
        .optsdcc -mz80 sdcccall(1)

        .globl  _yos_get_sys_info
        .globl  __yos_sys_info
        .globl  __sys_heap
        .globl  __bank_count
        .globl  _process_first
        .globl  _thread_current
        .globl  _thread_first_suspended
        .globl  _thread_first_running
        .globl  _thread_first_waiting
        .globl  _thread_first_terminated
        .globl  __tmr_first
        .globl  __evt_first
        .globl  __svc_first
        .globl  __library_private_services
        .globl  __library_refs

        .area   _CODE

        ; outputs: DE = const yos_sys_info_t descriptor in ROM
        ; clobbers: de; preserves af, bc, hl, ix and iy
_yos_get_sys_info::
        ld      de,#__yos_sys_info
        ret

        .area   _CONST
__yos_sys_info::
        .dw     __sys_heap
        .dw     0xc000                  ; banked heap address in every bank
        .dw     __bank_count
        .dw     _process_first
        .dw     _thread_current
        .dw     _thread_first_suspended
        .dw     _thread_first_running
        .dw     _thread_first_waiting
        .dw     _thread_first_terminated
        .dw     __tmr_first
        .dw     __evt_first
        .dw     __svc_first
        .dw     __library_private_services
        .dw     __library_refs
