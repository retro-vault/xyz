        ; Return the machine model detected during startup.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module yos_rom_model
        .optsdcc -mz80 sdcccall(1)

        .globl  _yos_rom_model
        .globl  __bank_model

        .area   _CODE

_yos_rom_model::
        ld      a,(__bank_model)
        ld      e,a
        ld      d,#0
        ret
