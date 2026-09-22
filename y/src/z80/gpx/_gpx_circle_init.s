        ; Shared midpoint-circle init for YOS.
        ; GPL2 License (see: LICENSE.libgpx)
        ; Copyright (C) 2026 Tomaz Stih

        .module _gpx_circle_init
        .optsdcc -mz80 sdcccall(1)
        .globl  __gpx_circle_init
        .area   _CODE

        ; Inputs: IX = circle frame; radius at +6/+7.
        ; Frame: xn=-5/-6, yn=-7/-8, decision f=-9/-10.
        ; Outputs: updated midpoint state in that frame.
        ; Clobbers: AF, DE, HL; preserves BC, IX, IY, alternates.
        ; No caller arguments consumed.
__gpx_circle_init::
        ;; xn = 0, yn = r
        xor     a
        ld      -5(ix),a
        ld      -6(ix),a
        ld      a,6(ix)
        ld      -7(ix),a
        ld      a,7(ix)
        ld      -8(ix),a

        ;; f = 1 - r
        ld      hl,#1
        ld      e,6(ix)
        ld      d,7(ix)
        sbc     hl,de                   ; carry already clear from xor a above
        ld      -9(ix),l
        ld      -10(ix),h

        ret
