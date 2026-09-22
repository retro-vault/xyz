        ; Shared midpoint-circle step x for YOS.
        ; GPL2 License (see: LICENSE.libgpx)
        ; Copyright (C) 2026 Tomaz Stih

        .module _gpx_circle_step_x
        .optsdcc -mz80 sdcccall(1)
        .globl  __gpx_circle_step_x
        .area   _CODE

        ; Inputs: IX = circle frame.
        ; Frame: xn=-5/-6, yn=-7/-8, decision f=-9/-10.
        ; Outputs: xn incremented; HL = updated f, also stored in frame.
        ; Clobbers: AF, DE, HL; preserves BC, IX, IY, alternates.
        ; No caller arguments consumed.
__gpx_circle_step_x::
        ld      l,-5(ix)
        ld      h,-6(ix)
        inc     hl
        ld      -5(ix),l
        ld      -6(ix),h
        ;; ddx = 2*xn + 1; xn is already in HL.
        add     hl,hl
        inc     hl
        ex      de,hl
        ld      l,-9(ix)
        ld      h,-10(ix)
        add     hl,de
        ld      -9(ix),l
        ld      -10(ix),h
        ret
