        ; Immutable service name placed in an otherwise unused vector gap.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _yos_name
        .optsdcc -mz80 sdcccall(1)

        .globl  __yos_name

        .area   _CODE
__yos_name::
        .asciz  "yos"
