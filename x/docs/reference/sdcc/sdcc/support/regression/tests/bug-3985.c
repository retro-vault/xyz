/*
   bug-3985.c
   An issue with dead-code elimination killing live code.
 */

#include <testfwk.h>

/// GPL 2.0 or later
#include <stdint.h>

#define u8 uint8_t
#define u16 uint16_t

typedef struct {
    u16 x;
} maths_point;

typedef struct {
    maths_point position;
} ShipData;

ShipData shipData;

const u8 playerColors[] = { 1 };

ShipData *PlayerGet(void) { return &shipData; }

#define SPRITE_WIDTH 16
#define SHIP_ENLARGE_WIDTH 24

#define TO_SCREEN(x) (x / 8)

u8 StateBonusModeGet(void);

void SpriteSet(uint8_t x, uint8_t colour);

void ShipDraw(void) {

    ShipData *ship = PlayerGet();

    // Ships
    u16 width = TO_SCREEN( SPRITE_WIDTH );
    u16 shipx = TO_SCREEN( ship->position.x );

    if ( StateBonusModeGet())
        width += TO_SCREEN( SHIP_ENLARGE_WIDTH );

    u16 shipxl = shipx;
    u16 shipxm = shipxl + width;
    u16 shipxr = shipxm + width;

    // Middle first to overlay the edges
    SpriteSet( shipxm, playerColors[0] );
}

void
testBug (void) {
    shipData.position.x = 0;
    ShipDraw();
}

u8 StateBonusModeGet(void)
{
    return 1;
}

void SpriteSet(uint8_t x, uint8_t colour)
{
    ASSERT (x == TO_SCREEN( 0 ) + TO_SCREEN( SPRITE_WIDTH ) + TO_SCREEN( SHIP_ENLARGE_WIDTH ));
    ASSERT (colour == 1);
}

