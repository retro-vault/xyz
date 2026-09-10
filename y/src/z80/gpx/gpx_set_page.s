        ;; gpx_set_page.s
        ;;
        ;; ZX Spectrum page selection API.
        ;; Current backend keeps this as a no-op.
        ;;
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-04-04   TS

        .module gpx_set_page
        .optsdcc -mz80 sdcccall(1)

        .globl  _gpx_set_page

        .area   _CODE

        ;; ------------------------------------------------------------
        ;; _gpx_set_page
        ;; Select the displayed page, the page drawn into, or both. The ZX
        ;; has a single framebuffer, so this does nothing here; the entry
        ;; exists so page-flipping code written against the API links and
        ;; runs unchanged on machines that do have more than one page.
        ;;
        ;; Signature:
        ;;   void gpx_set_page(uint8_t op, uint8_t page)
        ;;
        ;; Arguments:
        ;;   A = op flags (PG_DISPLAY, PG_WRITE, may be OR-ed)
        ;;   L = page number
        ;;
        ;; Clobbers:
        ;;   nothing
_gpx_set_page::
        ret
