        .module gpx_hide_sprite_next_l2
        .optsdcc -mz80 sdcccall(1)
        .globl _gpx_hide_sprite
        .globl _gpx_draw_bmp
        .area _CODE
_gpx_hide_sprite::
        push hl
        pop iy
        push de
        pop ix
        ld e,0(ix)
        ld d,1(ix)
        ld l,8(ix)
        ld h,9(ix)
        push hl                 ; clip
        ld l,6(ix)
        ld h,7(ix)
        ld a,h
        or l
        jr z,.empty
        push hl                 ; background bitmap
        ld l,2(ix)
        ld h,3(ix)
        push hl                 ; y
        push iy
        pop hl
        jp _gpx_draw_bmp
.empty:
        pop hl
        ret
