        ;; acoshl.s
        ;;
        ;; long double currently aliases double on this target, so acoshl()
        ;; simply reuses the double wrapper.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module acoshl
        .optsdcc -mz80 sdcccall(1)

        .globl  _acoshl
        .globl  _acosh

        .area   _CODE

_acoshl::
        jp      _acosh
