/*
 * Declares the low-level keyboard scanner and ring buffer used by the
 * tty layer to collect raw ZX Spectrum key events.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2021 tomaz stih
 */
#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include <stdint.h>

/*
 * Bit added to an internal key code when the event is a key-down event.
 */
#define KEY_DOWN_BIT	0b01000000

/*
 * Keyboard event ring buffer shared with the scanner.
 */
typedef struct kbd_buff_s {
	uint8_t start;
	uint8_t end;
	uint8_t count;
	uint8_t buffer[32];
} kbd_buff_t;

/*
 * Global keyboard event buffer filled by the scanner.
 */
extern kbd_buff_t* __kbd_buff;
#define _kbd_buff __kbd_buff

/*
 * Poll the keyboard matrix and queue state transitions.
 *
 * Notes:
 *      Intended to run from a 50 Hz timer hook.
 */
extern void __kbd_scan(void);
#define _kbd_scan __kbd_scan

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
/*
 * Read the next queued key event from the internal keyboard buffer.
 *
 * Returns:
 *      Zero when no event is available, otherwise one internal key code.
 */
extern uint8_t kbd_read(void);

#endif /* __KEYBOARD_H__ */
