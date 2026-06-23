/*
   bug-3981.c
   
 */

#include <testfwk.h>

#pragma disable_warning 85

#include <stdint.h>

typedef struct {

    uint8_t x;
    uint8_t y;

} maths_point_uint8_t;

uint8_t g_id = 3;

void sprite_set( uint8_t id, uint8_t tile, uint8_t x, uint8_t y, uint8_t colour ) {

    ASSERT( id == 3 );
}

void DrawSprite( uint8_t type, const maths_point_uint8_t *position, uint8_t angle ) {

    uint8_t colour = 7;
    uint8_t tile = angle / ( 256 / colour );

    if ( g_id >= 32 ) return;

    sprite_set( g_id++, tile * 4, position->x, position->y, colour );
}

void
testBug ( void ) {
    maths_point_uint8_t position = { 56, 98 };
    DrawSprite( 19, &position, 45 );
}

