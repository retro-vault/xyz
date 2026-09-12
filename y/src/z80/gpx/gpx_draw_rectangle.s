        ;; gpx_draw_rectangle.s -- legacy all-edge entry.
        ;; GPL2 License (see: LICENSE), Copyright (C) 2026 Tomaz Stih

        .module gpx_draw_rectangle
        .optsdcc -mz80 sdcccall(1)

        .globl  _gpx_draw_rectangle
        .globl  _gpx_draw_box

        .area   _CODE

        ;; ------------------------------------------------------------
        ;; void gpx_draw_rectangle(gpx_t *gpx, rect_t *r, color c,
        ;;     bmode m, uint8_t lpatt, const rect_t *clip)
        ;; Clobbers: AF, BC, DE, HL, IX, IY and the alternate set.
_gpx_draw_rectangle::
        ;; Insert the edge byte ahead of the existing arguments. The box
        ;; entry consumes it and returns directly to the original caller.
        pop     bc
        ld      a,#0x0f
        push    af
        inc     sp
        push    bc
        jp      _gpx_draw_box
