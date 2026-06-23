        ;; vla_zero.s — zero-initialize a VLA allocated on the stack.
        ;;
        ;; Called by C23 int vla[n] = {} to zero-initialize a VLA.
        ;; Interface (sdccall(0) stack ABI):
        ;;   arg0 (HL, pushed last) = pointer to first byte (vla base)
        ;;   arg1 (DE, pushed first) = byte count
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module vla_zero
        .globl __vla_zero

        .area _CODE

;;  void __vla_zero(void *ptr, unsigned int count)
;;  Zero count bytes starting at ptr.
__vla_zero:
        push    ix
        ld      ix, #0
        add     ix, sp
        ;; ptr   = 4(ix):5(ix)   (HL-sized, pushed last → highest address)
        ;; count = 6(ix):7(ix)
        ld      l, 4(ix)
        ld      h, 5(ix)
        ld      e, 6(ix)
        ld      d, 7(ix)
        ;; DE = count, HL = ptr
        ld      a, d
        or      e
        jr      z, .done        ; count == 0, nothing to do
.loop:
        ld      (hl), #0
        inc     hl
        dec     de
        ld      a, d
        or      e
        jr      nz, .loop
.done:
        ld      sp, ix
        pop     ix
        ret
