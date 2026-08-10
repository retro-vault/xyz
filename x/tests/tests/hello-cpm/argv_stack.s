        ; Present an ABI-0 implementation as the external C entry point.
        ; crt0 must supply the same argc/argv values on the stack that it also
        ; supplies in HL/DE for ABI 1.

        .module cpm_argv_stack
        .optsdcc -mz80 sdcccall(0)

        .globl  _main
        .globl  _stack_main

        .area   _CODE
_main::
        jp      _stack_main
