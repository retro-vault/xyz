        ;; _process_relocate.s
        ;;
        ;; XL relocator for process loader.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-04-12   tstih

        .module _process_relocate

        .globl  ___process_relocate

        .equ    XL_HDR_SIZE, 0x0c
        .equ    XL_OFF_RELOC_CNT, 0x08

        .area   _CODE
        ;; ------------------------------------------------------------
        ;; ___process_relocate
        ;; Walk relocation table and patch code in place.
        ;;
        ;; Signature:
        ;;   uint8_t __process_relocate(uint8_t *img)
        ;;
        ;; Arguments:
        ;;   HL = img pointer
        ;;
        ;; Returns:
        ;;   A = 0 on success, 1 on failure
        ;;
        ;; Clobbers:
        ;;   A, BC, DE, HL, IX
___process_relocate::
        push    ix
        push    hl
        pop     ix                      ; IX = img

        ld      c,8(ix)                ; BC = reloc_cnt
        ld      b,9(ix)

        ld      de,#XL_HDR_SIZE
        add     hl,de
        ex      de,hl                   ; DE = reloc_ptr

        ld      l,c
        ld      h,b
        add     hl,hl
        add     hl,hl
        add     hl,de
        push    hl
        pop     ix                      ; IX = code_ptr

.reloc_loop:
        ld      a,b
        or      c
        jr      z,.ok

        ld      a,(de)                  ; HL = off
        ld      l,a
        inc     de
        ld      a,(de)
        ld      h,a
        inc     de
        ld      a,(de)                  ; A = size
        inc     de
        inc     de                      ; skip reserved

        push    de
        push    bc
        push    ix
        pop     bc
        add     hl,bc                   ; HL = patch ptr

        cp      #2
        jr      z,.patch_word
        cp      #1
        jr      z,.patch_byte
        pop     bc
        pop     de
        ld      a,#1
        pop     ix
        ret

.patch_word:
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        push    hl
        push    ix
        pop     bc
        ex      de,hl
        add     hl,bc
        pop     de                      ; DE = addr of high byte
        ld      a,l
        dec     de
        ld      (de),a                  ; low byte
        inc     de
        ld      a,h
        ld      (de),a                  ; high byte
        jr      .next

.patch_byte:
        ld      a,(hl)
        push    ix
        pop     bc
        add     a,c                     ; + low byte of code_ptr
        ld      (hl),a

.next:
        pop     bc
        pop     de
        dec     bc
        jr      .reloc_loop

.ok:
        xor     a
        pop     ix
        ret
