        ;; Unavailable operation: null pointer/word and zero byte result.
        ;; MIT License (see LICENSE), Copyright (C) 2026 Tomaz Stih.
        .module return_null
        .optsdcc -mz80 sdcccall(1)
        .area _CODE
__yos_return_null::
        xor a
        ld de,#0
        ret
