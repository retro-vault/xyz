        ;; libc_align_size.s
        ;; Split from heap_core.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module libc_align_size
        .optsdcc -mz80 sdcccall(1)

        .globl  __libc_align_size
        .globl  __libc_heap_split

BLOCK_HDR_SIZE  .equ 8
BLOCK_HEAP_HI   .equ 7
BLOCK_HEAP_LO   .equ 6
BLOCK_NEXT_HI   .equ 5
BLOCK_NEXT_LO   .equ 4
BLOCK_SIZE_HI   .equ 1
BLOCK_SIZE_LO   .equ 0

        .area   _CODE
__libc_align_size::
        inc     hl
        res     0, l
        ret

;; Convert a user payload pointer back to the preceding block header.
__libc_heap_split::
        push    bc
        ld      b,h
        ld      c,l                     ; BC = requested size
        ld      h,b
        ld      l,c
        ld      de,#(BLOCK_HDR_SIZE + 1)
        add     hl,de                   ; HL = requested size + spill threshold
        ld      e,BLOCK_SIZE_LO(ix)
        ld      d,BLOCK_SIZE_HI(ix)
        ex      de,hl                   ; DE = requested+threshold, HL = block size
        or      a
        sbc     hl,de                   ; block size - minimum tail payload
        jr      c,heap_split_done
        jr      z,heap_split_done

        push    ix
        pop     hl
        ld      de,#BLOCK_HDR_SIZE
        add     hl,de
        add     hl,bc                   ; HL = tail header address
        push    hl                      ; keep tail header address live

        ld      l,BLOCK_SIZE_LO(ix)
        ld      h,BLOCK_SIZE_HI(ix)
        or      a
        sbc     hl,bc
        ld      de,#BLOCK_HDR_SIZE
        or      a
        sbc     hl,de                   ; HL = tail->size
        ex      de,hl
        pop     hl                      ; HL = tail header address
        push    hl                      ; preserve for block->next update
        ld      (hl),e
        inc     hl
        ld      (hl),d
        inc     hl
        ld      (hl),#1                 ; tail->free = 1
        inc     hl
        xor     a
        ld      (hl),a
        inc     hl
        ld      a,BLOCK_NEXT_LO(ix)
        ld      (hl),a                  ; tail->next low
        inc     hl
        ld      a,BLOCK_NEXT_HI(ix)
        ld      (hl),a                  ; tail->next high
        inc     hl
        ld      a,BLOCK_HEAP_LO(ix)
        ld      (hl),a                  ; tail->heap low (inherit owner)
        inc     hl
        ld      a,BLOCK_HEAP_HI(ix)
        ld      (hl),a                  ; tail->heap high

        ld      BLOCK_SIZE_LO(ix),c
        ld      BLOCK_SIZE_HI(ix),b
        pop     hl
        ld      BLOCK_NEXT_LO(ix),l
        ld      BLOCK_NEXT_HI(ix),h

heap_split_done:
        pop     bc
        ret

;; After a free, walk the list and join adjacent free blocks back together.
