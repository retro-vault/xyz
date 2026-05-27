/** bug-3831.c: a code generation bug for z80-related targets.
  An equality comparison could overwrite a non-dead value in register a.
 */

#include <testfwk.h>

typedef unsigned char u8;

static u8 gGlobal;
static u8 GetValue(void) { return 0xa5; }
static void UseValue(u8 v) { ASSERT (v == 0xa5); }

static void Bug(void)
{
    u8 v;

    v = GetValue();

    if (v != gGlobal)
    {
        // At this point, "v" is clobbered and invalid
        UseValue(v);
    }
}

void testBug (void)
{
    gGlobal = 0x5a;
    Bug ();
}

