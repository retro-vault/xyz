        ;; crt0tiny.s
        ;;
        ;; Minimal runtime for tiny step-test apps.
        ;; Same as tests/hello/crt0.s; keep YOS-provided thread stack intact.

        .module   crt0tiny

        .globl    _main
        .globl    _entry
        .globl    _query_service
        .globl    _query_interface
        .globl    ___sdcc_call_hl
        .globl    ___sdcc_call_iy

        .area     _CODE
_entry::
        call      _main
        ret

_query_service::
_query_interface::
        rst       0x10
        ret

___sdcc_call_hl::
        jp        (hl)

___sdcc_call_iy::
        push      iy
        ret

        .area     _GSINIT
        .area     _GSFINAL
        .area     _DATA
        .area     _INITIALIZED

        .area     _BSS

        .area     _HEAP
        .area     _INITIALIZER
