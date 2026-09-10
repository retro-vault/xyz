        ; Link roots for assembly-kernel APIs exercised by test_kernel.cpp.
        ; Production ROMs omit these roots so xld can discard unused APIs.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module test_roots
        .optsdcc -mz80 sdcccall(1)

        .globl  _process_start
        .globl  _boot_shell

        .area   _CODE

        ; Unit tests run without esxDOS; suppress the production disk boot.
_boot_shell::
        ld      de,#0
        ret

        .area   _CONST

        ; Keep process construction and its dependencies in the test ROM.
        .dw     _process_start
