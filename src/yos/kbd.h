/*
 * kbd.h
 *
 * definitions for low level keyboard scanning.
 * 
 * MIT License (see: LICENSE)
 * copyright (C) 2021 tomaz stih
 *
 * 2012-06-26   tstih
 *
 */
#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include <stdint.h>

#define KEY_DOWN_BIT	0b01000000

typedef struct kbd_buff_s {
	uint8_t start;
	uint8_t end;
	uint8_t count;
	uint8_t buffer[32];
} kbd_buff_t;

/* keyboard buffer */
extern kbd_buff_t* kbd_buff;

/* call this function 50 times per second */
extern void kbd_scan();

/* checks kbd. buffer, 0 for no key available 
   otherwise internal key code 
   
   internal key code to zx spectrum key map:

    '5', '4', '3', '2', '1'
    '6', '7', '8', '9', '0'
    'y', 'u', 'i', 'o', 'p'
    'h', 'j', 'k', 'l', <enter>	
    'b', 'n', 'm', <symbol shift>, <space>
    'v', 'c', 'x', 'z', <caps shift>
    'g', 'f', 'd', 's', 'a'
    't', 'r', 'e', 'w', 'q'
   
   all key down and key up events are buffered,
   the tty should keep track of shift up/down
   events and them convert the key into correct
   ascii code based on it! */
extern uint8_t kbd_read();

#endif /* __KEYBOARD_H__ */