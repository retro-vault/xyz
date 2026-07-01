        ;; string_return_clean.s
        ;; Shared sdcccall(1) callee-clean return helpers for string routines
        ;; that still carry stack-passed arguments after popping IX.

        .module string_return_clean
        .optsdcc -mz80 sdcccall(1)

        .globl  __string_ret_clean2
        .globl  __string_ret_clean4
        .globl  __string_return_zero_clean2
        .globl  __string_return_zero_clean4
        .globl  __string_return_hl_clean2
        .globl  __string_return_hl_clean4
        .globl  __string_return_bc_clean2
        .globl  __string_return_bc_clean4

        .area   _CODE
__string_return_zero_clean2::
        ld      de,#0x0000
        jp      __string_ret_clean2

__string_return_zero_clean4::
        ld      de,#0x0000
        jp      __string_ret_clean4

__string_return_hl_clean2::
        ex      de,hl
        jp      __string_ret_clean2

__string_return_hl_clean4::
        ex      de,hl
        jp      __string_ret_clean4

__string_return_bc_clean2::
        ld      d,b
        ld      e,c
        jp      __string_ret_clean2

__string_return_bc_clean4::
        ld      d,b
        ld      e,c
        jp      __string_ret_clean4

__string_ret_clean2::
        pop     bc
        inc     sp
        inc     sp
        push    bc
        ret

__string_ret_clean4::
        pop     bc
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        push    bc
        ret
