        ; xcc indirect call trampoline
        ; Jumps to the function pointer currently held in HL.
        ; This is kept as its own module so executable tests only link
        ; it
        ; when
        ; function pointers are actually used.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module call_hl_bridge
        .area   _CODE
        .globl  __call_hl

        ; __call_hl
        ; inputs: HL = function pointer target.
        ; outputs: control jumps to the pointed function.
        ; clobbers: none in the trampoline itself.

__call_hl:
        jp      (hl)
