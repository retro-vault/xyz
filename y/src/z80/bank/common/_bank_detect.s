        ; Detect the machine and install its fixed-RAM bank-map trampoline.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _bank_detect
        .optsdcc -mz80 sdcccall(1)

        .globl  __bank_detect
        .globl  __bank_map
        .globl  __bank_probe
        .globl  __bank_count
        .globl  __bank_model

        .equ    YOS_ROM_MODEL_48K,       0
        .equ    YOS_ROM_MODEL_128K,      1
        .equ    YOS_ROM_MODEL_NEXT,      2

        .area   _CODE

        ; The probe returns A=model, B=usable banks and HL=mapper. Keep this
        ; small front end separate so the linker can place it in the otherwise
        ; unusable bytes immediately before the divIDE 04C6h paging trap.
__bank_detect::
        call    __bank_probe
        ld      (__bank_model),a
        ld      a,b
        ld      (__bank_count),a
        ld      a,#0xc3                  ; JP nn
        ld      (__bank_map),a
        ld      (__bank_map+1),hl
        xor     a
        jp      __bank_map              ; initialize mapper state at bank 0
