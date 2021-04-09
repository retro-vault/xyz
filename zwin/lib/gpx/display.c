#include "display.h"

display_t display;

display_t* display_init(void *di) {
    display.xmin=display.ymin=0;
    display.xmax=XMAX;
    display.ymax=YMAX;
    display.display_info=di;
    return &display;
}