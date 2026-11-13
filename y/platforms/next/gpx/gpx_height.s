        .module gpx_height_next_l2
        .optsdcc -mz80 sdcccall(1)
        .globl _gpx_height
        .area _CODE
_gpx_height::
        ld de,#256
        ret
