        ;; asinhl.s
        ;;
        ;; long double currently aliases double on this target, so asinhl()
        ;; simply reuses the double wrapper.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module asinhl
        .optsdcc -mz80 sdcccall(1)

        .globl  _asinhl
        .globl  _asinh

        .area   _CODE

_asinhl::
        jp      _asinh
