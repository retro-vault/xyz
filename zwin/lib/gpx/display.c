#include "display.h"

display_t display;

display_t* display_init(void *di) {
    display.xmin=display.ymin=0;
    display.xmax=SCREEN_WIDTH-1;
    display.ymax=SCREEN_HEIGHT-1;
    display.display_info=di;
    return &display;
}