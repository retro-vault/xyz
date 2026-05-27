        ; Minimal executable-test startup for the GNU assembler/linker
        ; flow.
        ; Runs _main, stores the 16-bit return value into the fixed
        ; mailbox at
        ; 0xff00, writes the completion byte at 0xff02, and halts in a
        ; loop.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .text
        .global __xc_test_start
        .global _main

        ; __xc_test_start
        ; inputs: none.
        ; outputs: does not return; writes the mailbox and halts.
        ; clobbers: af, hl, sp.

__xc_test_start:
        ld      sp, 0xf000
        call    _main
        ld      (0xff00), hl
        ld      a, 0xa5
        ld      (0xff02), a
1:
        halt
        jr      1b
