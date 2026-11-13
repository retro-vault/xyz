        ; Release the fixed-image address-preservation prefix in place.
        ; MIT License (see: LICENSE), Copyright (C) 2026 tomaz stih
        .module _image_retain
        .optsdcc -mz80 sdcccall(1)
        .globl __image_retain, __mem_split, __os_free
        .globl _enter_critical_section, _leave_critical_section
        .area _CODE
        ; IX=loader frame: +66 original payload, +68 resident code. Fixed
        ; images reserve twelve bytes before the code so their base matches
        ; the former contiguous XL layout. Splitting after five payload bytes
        ; puts the tail payload exactly at code (+5 payload, +7 header).
        ; Clobbers AF/BC/DE/HL; preserves IX/IY; no stack arguments.
__image_retain::
        call _enter_critical_section
        ld l,66(ix)
        ld h,67(ix)
        push hl                         ; original prefix payload
        ld bc,#-7
        add hl,bc
        push ix                         ; loader frame
        push hl
        pop ix
        ld de,#5
        call __mem_split
        pop ix
        pop hl                          ; original prefix payload
        jr c,.unsplit
        ld e,68(ix)
        ld d,69(ix)
        ld 66(ix),e
        ld 67(ix),d
        call _leave_critical_section
        jp __os_free
.unsplit:
        jp _leave_critical_section
