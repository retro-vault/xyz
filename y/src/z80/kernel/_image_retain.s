        ; Retain loaded code in place and release the metadata allocation.
        ; MIT License (see: LICENSE), Copyright (C) 2026 tomaz stih
        .module _image_retain
        .optsdcc -mz80 sdcccall(1)
        .globl __image_retain, __mem_split, __yos_shrink
        .globl _enter_critical_section, _leave_critical_section
        .area _CODE
        ; IX=loader frame: +12 relocated code end, +66 future resident,
        ; +80 original allocation.
        ; The XL relocation table trails the code, so shrink the owned
        ; buffer to the code end first (the public shrink_memory path); then
        ; split the buffer at the compact exports/code boundary. The old
        ; allocation now covers only the metadata prefix, and the loader's
        ; final cleanup frees it through the unchanged +80 pointer. A
        ; fragment too small for a heap block simply stays resident.
        ; Metadata must no longer be referenced.
        ; Clobbers AF/BC/DE/HL; preserves IX/IY; no stack arguments.
__image_retain::
        call _enter_critical_section
        push ix                         ; loader frame
        ld e,80(ix)
        ld d,81(ix)                     ; original payload
        ld l,66(ix)
        ld h,67(ix)                     ; future resident
        or a
        sbc hl,de
        ld bc,#-7
        add hl,bc                       ; discarded prefix payload size
        push hl
        ld l,12(ix)
        ld h,13(ix)                     ; relocated code end
        or a
        sbc hl,de                       ; payload retained through the code
        ex de,hl                        ; HL = original payload, DE = retained size
        call __yos_shrink               ; release the consumed relocation table
        ex de,hl                        ; DE = payload again: the block is live
        ld bc,#-7
        add hl,bc                       ; original allocation header
        push hl
        pop ix
        pop de                          ; discarded prefix payload size
        call __mem_split                ; both fragments retain their owner
        pop ix                          ; loader frame
        jp _leave_critical_section
