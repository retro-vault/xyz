        ; Thread-local storage base fallback for the merged xcc runtime.
        ; A threaded host can override this archive member with its own
        ; implementation.  A single-threaded program uses the linked TLS
        ; initialization template itself as its live TLS block.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module tls_base
        .area   _CODE
        .globl  __tls_base
        .globl  __tls_template

        ; inputs: none.
        ; outputs: HL = pointer to the current thread TLS block
        ; clobbers: HL.

__tls_base:
        ld      hl, #__tls_template
        ret
