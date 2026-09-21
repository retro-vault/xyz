        ; Replacement-ROM character output hook for esxDOS dot commands.
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module esx_print
        .optsdcc -mz80 sdcccall(1)
        .globl  __esx_print
        .globl  __esx_print_hook
        .globl  __zx_esx_gate_8f

        .area   _BSS
__esx_print_hook:: .ds 2

        .area   _CODE
        ; Called with character in A through the fixed 09F4h entry. This entry is also safe
        ; during esxDOS cold boot, before YOS has initialized RAM: the gate
        ; signature must match before the hook pointer is consulted.
        ; Preserve AF and pass the byte in A; HL is scratch as in ROM PRINT-OUT.
__esx_print::
        push    af
        ld      hl,(__zx_esx_gate_8f)
        ld      a,l
        cp      #0xcf
        jr      nz,.done
        ld      a,h
        cp      #0x8f
        jr      nz,.done
        ld      a,(__zx_esx_gate_8f+2)
        cp      #0xc9
        jr      nz,.done
        ld      hl,(__esx_print_hook)
        ld      a,h
        or      l
        jr      z,.done
        pop     af
        jp      (hl)
.done:
        pop     af
        ret
