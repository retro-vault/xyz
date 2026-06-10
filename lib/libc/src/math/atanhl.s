        ;; atanhl.s
        ;;
        ;; long double currently aliases double on this target, so atanhl()
        ;; simply reuses the double wrapper.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module atanhl
        .optsdcc -mz80 sdcccall(1)

        .globl  _atanhl
        .globl  _atanh

        .area   _CODE

_atanhl::
        jp      _atanh
