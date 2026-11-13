        ;; Three-byte whole-pixel operations, patched before Layer 2 mapping.
        ;; The helper and table can therefore live below the overlay window.
        .module next_blit_ops
        .optsdcc -mz80 sdcccall(1)
        .globl __next_blit_prepare, _next_blit_args
        .globl __next_blit_ink_a, __next_blit_ink_b, __next_blit_paper_a, __next_blit_paper_b
        .area _CODE
__next_blit_prepare::
        ld a,(_next_blit_args+12)
        ld de,#__next_blit_ink_a
        call .patch
        ld a,(_next_blit_args+12)
        ld de,#__next_blit_ink_b
        call .patch
        ld a,(_next_blit_args+13)
        ld de,#__next_blit_paper_a
        call .patch
        ld a,(_next_blit_args+13)
        ld de,#__next_blit_paper_b
        jp .patch
;; A=kind (0 skip, 1 black, 2 white, 3 invert), DE=block to patch.
.patch:
        ld c,a
        add a,a
        add a,c
        ld c,a
        ld b,#0
        ld hl,#.operations
        add hl,bc
        ld bc,#3
        ldir
        ret
        .area _HEADER_DATA
.operations:
        .db 0x00,0x00,0x00       ; skip: NOPs
        .db 0x36,0x00,0x00       ; black: LD (HL),0; NOP
        .db 0x36,0xff,0x00       ; white: LD (HL),FF; NOP
        .db 0x7e,0x2f,0x77       ; invert: LD A,(HL); CPL; LD (HL),A
