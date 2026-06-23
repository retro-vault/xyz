        ;; unreachable.s — __builtin_unreachable() runtime stub for xcc/Z80.
        ;;
        ;; Called when execution reaches code that should never be reached.
        ;; HALT stops the Z80 CPU until the next interrupt or NMI.
        ;; The infinite loop after HALT makes the intent clear if HALT is
        ;; somehow resumed (e.g. by an NMI in an unusual system).
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module unreachable
        .globl __builtin_unreachable

        .area _CODE

__builtin_unreachable:
        halt
        jp      __builtin_unreachable
