        ; Shared midpoint-circle step y for YOS.
        ; GPL2 License (see: LICENSE.libgpx)
        ; Copyright (C) 2026 Tomaz Stih

        .module _gpx_circle_step_y
        .optsdcc -mz80 sdcccall(1)
        .globl  __gpx_circle_step_y
        .area   _CODE

        ; Inputs: IX = circle frame.
        ; Frame: xn=-5/-6, yn=-7/-8, decision f=-9/-10.
        ; Outputs: yn decremented; HL = updated f, also stored in frame.
        ; Clobbers: AF, DE, HL; preserves BC, IX, IY, alternates.
        ; No caller arguments consumed.
__gpx_circle_step_y::
        ld      l,-7(ix)
        ld      h,-8(ix)
        dec     hl
        ld      -7(ix),l
        ld      -8(ix),h
        ;; f -= 2*yn; yn is already in HL.
        add     hl,hl
        ex      de,hl
        ld      l,-9(ix)
        ld      h,-10(ix)
        or      a
        sbc     hl,de
        ld      -9(ix),l
        ld      -10(ix),h

        ret
