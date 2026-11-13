        ;; Shared entry maps the single resident ISR and restores the interrupted bank.
        .module partner_yos_irq
        .globl __serial_interrupt_prepare, __serial_interrupt_restore_gateway
        .globl __partner_gateway, __partner_bank_calls_ready
        .globl __partner_bank_select
        .globl __serial_interrupt_dispatch, __serial_interrupt_stack_top
        .area _BSS
__partner_gateway:: .ds 2
__partner_bank_calls_ready:: .ds 1
.saved_sp: .ds 2
        .area _CODE
__serial_interrupt_prepare::
        ld hl,#.irq
        ld (__partner_gateway),hl
        ret
__serial_interrupt_restore_gateway::
        ret
.irq:
        push af
        ld a,(0xc002)
        push af
        ld a,#1
        call __partner_bank_select
        ld (.saved_sp),sp
        ld sp,(__serial_interrupt_stack_top)
        ; Direct call in the mapping selected above. A kernel-call veneer
        ; would overwrite the interrupted alternate registers before saving.
        call __serial_interrupt_dispatch
        ld sp,(.saved_sp)
        pop af
        call __partner_bank_select
        pop af
        ei
        reti

        ; ALTO drawing runs cooperatively; the serial ISR never paints.
        .globl _enter_critical_section, _leave_critical_section
_enter_critical_section::
_leave_critical_section::
        ret
