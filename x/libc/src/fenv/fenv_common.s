        ; fenv_common.s
        ;
        ; Shared floating-point environment state for the xcc Z80 libc.
        ; One process-wide fenv_t { fexcept_t excepts; int rounding; } plus the
        ; public default environment.  fexcept_t is 2 bytes, so the struct is
        ; 4 bytes: excepts at offset 0, rounding at offset 2.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih



        .module fenv_common
        .optsdcc -mz80 sdcccall(1)

        .globl  __fe_current_env

        .area   _DATA
__fe_current_env::
        .dw     0                       ; excepts
        .dw     0                       ; rounding (FE_TONEAREST)

