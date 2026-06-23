        ; Thread-local storage base stub for the merged xcc runtime.
        ; Returns NULL until the host operating system overrides it.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        ; The final system should replace this helper with code that
        ; returns the active thread TLS base address in HL.

        .module tls_base
        .area   _CODE
        .globl  __tls_base

        ; NULL.
        ; inputs: none.
        ; outputs: HL = pointer to the current thread TLS block, or
        ; clobbers: HL.

__tls_base:
        ; stub returns NULL
        ld      hl, #0
        ret
