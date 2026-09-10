        ;; gpx_destroy.s
        ;;
        ;; ZX Spectrum GPX destroy routine.
        ;;
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-03-29   TS

        .module gpx_destroy
        .optsdcc -mz80 sdcccall(1)

        .globl  _gpx_destroy
        .globl  __gpx_ctx

        .area   _CODE

        ;; ------------------------------------------------------------
        ;; _gpx_destroy
        ;; Release the graphics subsystem. The ZX context is static, so
        ;; there is nothing to free; the entry exists so callers have one
        ;; shutdown point and the API matches every other backend.
        ;;
        ;; Signature:
        ;;   void gpx_destroy(gpx_t *gpx)
        ;;
        ;; Arguments:
        ;;   HL = gpx (ignored)
        ;;
        ;; Clobbers:
        ;;   nothing
_gpx_destroy::
        ret
