        .module gpx_show_sprite_next_l2
        .optsdcc -mz80 sdcccall(1)
        .globl _gpx_show_sprite
        .globl _gpx_draw_bmp
        .area _CODE
_gpx_show_sprite::
        push hl
        pop iy                  ; context
        push de
        pop ix                  ; sprite descriptor
        ld e,0(ix)
        ld d,1(ix)              ; x
        ld l,8(ix)
        ld h,9(ix)
        push hl                 ; clip (last argument)
        ld l,4(ix)
        ld h,5(ix)
        push hl                 ; bitmap
        ld l,2(ix)
        ld h,3(ix)
        push hl                 ; y
        push iy
        pop hl                  ; gpx
        jp _gpx_draw_bmp
