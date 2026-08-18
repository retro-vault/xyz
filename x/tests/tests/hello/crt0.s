        ;; crt0.s
        ;;
        ;; Vanilla Z80 startup code for relocatable user programs.
        ;; Under YOS the kernel/thread startup already owns SP, so we only
        ;; call _main and return to the thread shim.
        ;;
        ;; This is intentionally minimal: no BSS clearing, no
        ;; initialized-data copy. Suitable only for programs with
        ;; no global variables (like the hello.c integration test).
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-04-01   tstih

        .module   crt0

        .globl    _main
        .globl    _entry
        .globl    _query_service
        .globl    _query_interface
        .globl    ___sdcc_call_hl
        .globl    __sdcc_call_hl
        .globl    ___sdcc_call_iy
        .globl    __sdcc_call_iy

        ;; _CODE must be the first area so _entry lands at offset 0.
        ;; The OS loader uses the XL header entry_point to jump here.
        .area     _CODE
_entry::
        call      _main                   ; call C main
        ret                               ; return to thread startup shim

        ;; ------------------------------------------------------------
        ;; query_service
        ;; Dispatch strategy:
        ;;   use RST10 vector to resolve a named OS service table.
        ;;
        ;; Signature:
        ;;   void *query_service(char *name)
        ;;
        ;; Arguments:
        ;;   HL = pointer to zero-terminated service name
        ;;
        ;; Returns:
        ;;   DE = service pointer (or 0), matching sdcccall(1)
_query_service::
_query_interface::
        rst       0x10                    ; kernel returns pointer in DE
        ret

        ;; ------------------------------------------------------------
        ;; ___sdcc_call_hl
        ;; Dispatch strategy:
        ;;   SDCC helper for indirect calls through function pointers.
        ;;
        ;; Signature:
        ;;   helper ABI (called by SDCC-generated code)
        ;;
        ;; Arguments:
        ;;   HL = function pointer target
        ;;
        ;; Returns:
        ;;   returns as if target was called directly
___sdcc_call_hl::
__sdcc_call_hl::
        jp        (hl)

___sdcc_call_iy::
__sdcc_call_iy::
        push      iy
        ret

        ;; Declare remaining SDCC areas in link order.
        ;; hello.rel may contribute _DATA / _BSS sections.
        .area     _GSINIT
        .area     _GSFINAL
        .area     _DATA
        .area     _INITIALIZED

        ;; No private stack here: YOS thread runtime provides the stack.
        .area     _BSS

        .area     _HEAP
        .area     _INITIALIZER
