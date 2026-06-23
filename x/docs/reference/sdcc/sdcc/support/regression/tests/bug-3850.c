/* bug-3850.c
   An issue in handling of non-dead register a in z80 codegen for casts to bool.
 */
 
#include <testfwk.h>

#include <stdbool.h>

struct {
    bool b : 1;
} x;
bool f( void ) { return true; }
int m( void ) {
    x.b |= f();
    return 0;
}

void testBug (void) {
    x.b = false;
    m();
    ASSERT(x.b);
}

