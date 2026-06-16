        ;; libc_ptr_to_block.s
        ;; Split from heap_core.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module libc_ptr_to_block
        .optsdcc -mz80 sdcccall(1)

        .globl  __libc_heap_unwrap_user
        .globl  __libc_ptr_to_block

ALIGNED_MAGIC_HI .equ 0xa1
ALIGNED_MAGIC_LO .equ 0x6c

        .area   _CODE
        ;; Convert a user payload pointer back to its 8-byte block header.
        ;; Flag-neutral (callers may rely on flags set before the call).
__libc_ptr_to_block::
        dec     hl
        dec     hl
        dec     hl
        dec     hl
        dec     hl
        dec     hl
        dec     hl
        dec     hl
        ret

;; aligned_alloc() prefixes its user pointer with:
;;   [-6,-5] requested size
;;   [-4,-3] magic tag
;;   [-2,-1] original malloc() pointer
;; Free always accepts the visible user pointer, while realloc() needs both
;; the original base block and the caller-visible address.
;;   HL = user pointer
;;   HL = original malloc pointer when tagged, unchanged otherwise
;;   carry set when the pointer came from aligned_alloc()
__libc_heap_unwrap_user::
        ld      a,h
        or      l
        ret     z
        push    hl
        ld      de,#4
        or      a
        sbc     hl,de
        ld      a,(hl)
        cp      #ALIGNED_MAGIC_LO
        jr      nz,heap_unwrap_plain
        inc     hl
        ld      a,(hl)
        cp      #ALIGNED_MAGIC_HI
        jr      nz,heap_unwrap_plain
        inc     hl
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        pop     hl
        ld      l,e
        ld      h,d
        scf
        ret
heap_unwrap_plain:
        pop     hl
        scf
        ccf
        ret
