        ; Probe Spectrum banking hardware and select its ROM mapper.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _bank_probe
        .optsdcc -mz80 sdcccall(1)

        .globl  __bank_probe
        .globl  __bank_map_48
        .globl  __bank_map_128
        .globl  __bank_map_next

        .equ    YOS_ROM_MODEL_48K,       0
        .equ    YOS_ROM_MODEL_128K,      1
        .equ    YOS_ROM_MODEL_NEXT,      2

        .area   _CODE

        ; Output: A=model, B=usable bank count, HL=selected mapper.
        ; NextReg/MMU is checked first. Otherwise temporarily write one byte
        ; in 7FFD pages 0 and 1; both bytes and bank zero are restored. All
        ; 128K writes retain ROM-select bit 4 because ESXIDE boots YOS there.
__bank_probe::
        ld      bc,#0x243b
        xor     a
        out     (c),a                   ; select NextReg machine ID
        inc     b
        in      a,(c)
        cp      #0x08                   ; emulator
        jr      z,.next
        cp      #0x0a                   ; ZX Spectrum Next
        jr      z,.next
        cp      #0xfa                   ; anti-brick core
        jr      z,.next
        call    .detect_128
        jr      nz,.model_128

        xor     a
        ld      b,#1
        ld      hl,#__bank_map_48
        ret

.model_128:
        ld      a,#YOS_ROM_MODEL_128K
        ld      b,#YOS_BANK_COUNT_128
        ld      hl,#__bank_map_128
        ret

.next:
        ld      a,#YOS_ROM_MODEL_NEXT
        ld      b,#YOS_BANK_COUNT
        ld      hl,#__bank_map_next
        ret

        ; NZ when 7FFD exposes distinct pages 0 and 1. Only page zero is
        ; modified; its marker is chosen as the inverse of the untouched
        ; page-one byte, making the result deterministic. Bank zero and its
        ; original byte are restored before returning.
.detect_128:
        ld      bc,#0x7ffd
        ld      a,#0x10
        out     (c),a
        ld      hl,#0xc000
        ld      d,(hl)                  ; original page-0 byte
        inc     a
        out     (c),a
        ld      a,(hl)                  ; untouched page-1 byte
        cpl
        ld      e,a                     ; guaranteed-different marker
        ld      a,#0x10
        out     (c),a
        ld      (hl),e
        inc     a
        out     (c),a
        ld      a,(hl)
        cp      e
        push    af                      ; preserve paging-test result
        ld      a,#0x10
        out     (c),a
        ld      (hl),d
        pop     af
        ret
