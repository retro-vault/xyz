        ;; calloc.s
        ;;
        ;; Multiply two 16-bit allocation dimensions with overflow detection,
        ;; then obtain a zero-filled block from Alto's compact shared heap.
        ;; MIT License (see LICENSE), Copyright (C) 2026 Tomaz Stih.

        .module alto_calloc
        .optsdcc -mz80 sdcccall(1)
        .globl _malloc, __alloc_zero
        .area _CODE
        .globl  _calloc
        ;; ------------------------------------------------------------
        ;; _calloc
        ;; General calloc, without importing multiply and divide helpers.
        ;; Signature: void *calloc(uint16_t count, uint16_t size)
        ;; Arguments: HL=count, DE=element bytes
        ;; Returns: DE=cleared storage, or NULL on overflow/allocation failure
        ;; Clobbers: AF, BC, DE, HL
        ;; References: _malloc, __alloc_zero
        ;;
        ;; HL=count, DE=element bytes -> DE=cleared storage or zero.
        ;; Shift the count down; only double the element size while bits remain.
        ;; Either ADD carry means the true product cannot fit the 16-bit arena.
        ;; The common SDK call calloc(1,n) takes one iteration and no shifts.
_calloc::
        ld      b,h
        ld      c,l
        ld      hl,#0
.heap_product:
        srl     b
        rr      c
        jr      nc,.heap_product_skip
        add     hl,de
        jr      c,.heap_product_overflow
.heap_product_skip:
        ld      a,b
        or      c
        jr      z,.heap_product_ready
        sla     e
        rl      d
        jr      nc,.heap_product
.heap_product_overflow:
        ld      de,#0
        ret
.heap_product_ready:
        ld      a,h
        or      l
        jp      z,_malloc               ; zero-sized allocation clears no bytes
        jp      __alloc_zero
