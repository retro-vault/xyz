        ;; The YOS backend replaces malloc/free/realloc with kernel-backed
        ;; implementations.  Keep the generic heap hook as an empty region for
        ;; programs that reference the extension explicitly.

        .module yos_heap_region
        .optsdcc -mz80 sdcccall(1)
        .globl  _heap_region
        .area   _CODE
_heap_region::
        ld      hl,#0
        ld      de,#0
        ret
