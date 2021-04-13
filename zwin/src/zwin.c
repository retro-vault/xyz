/*
 *	zwin.c
 *	z windows main file.
 *
 *	tomaz stih tue apr 13 2021
 */
#include "zwin.h"

void zwin_init() {
    
    /* initialize messaging system
     * this will create the message queue.
     */
    message_init();

    /* initialize windowing system
     * this will initialize graphics and create
     * desktop window
     */
    window_init();

    /* initialize mouse 
     * this will calibrate the mouse to the center 
     * of the screen and execute first mouse hw scan.
    */
    mouse_init();
}

/* harvest and dispatch events
 * tis is zwin's main loop. it harvests events and
 * dispatches them to correct windows' message queues.
 */
void zwin_main() {

}