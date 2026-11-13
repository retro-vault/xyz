        .module gpx_width_next_l2
        .optsdcc -mz80 sdcccall(1)
        .globl _gpx_width
        .area _CODE
_gpx_width::
        ld de,#320
        ret
