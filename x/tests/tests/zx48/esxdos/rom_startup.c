/* ROM boot relocation: writable data, pointers, BSS, initializer storage,
 * an extra GSINIT contribution, and constants across divIDE trap addresses. */
#include <sys/esxdos.h>

extern const unsigned char zx_rom_pattern[];
extern volatile unsigned zx_rom_initialized;
extern volatile unsigned zx_rom_init_hook;

volatile unsigned zx_disk_result;
volatile unsigned zx_disk_phase;
static volatile unsigned initialized_word = 0x7351;
static const unsigned char * volatile initialized_pointer = zx_rom_pattern;
static volatile unsigned char cleared[37];

#define CHECK(n, condition) do { zx_disk_phase = (n); if (!(condition)) { \
    zx_disk_result = (n); for (;;) {} } } while (0)

int main(void)
{
    unsigned i;

    CHECK(1, initialized_word == 0x7351);
    CHECK(2, initialized_pointer == zx_rom_pattern);
    CHECK(3, zx_rom_initialized == 0xc3d2);
    CHECK(4, zx_rom_init_hook == 0x6b19);
    for (i = 0; i < sizeof cleared; ++i)
        CHECK(5, cleared[i] == 0);
    CHECK(6, zx_rom_pattern[0] == 0xa6 && zx_rom_pattern[1] == 0x59);
    for (i = 2; i < 12286; ++i)
        CHECK(7, zx_rom_pattern[i] == 0);
    CHECK(8, zx_rom_pattern[12286] == 0x3c
          && zx_rom_pattern[12287] == 0xc3);
    CHECK(9, zx_esxdos_version() >= 0x0890);
    initialized_word = 0x194b;
    CHECK(10, initialized_word == 0x194b);
    zx_disk_result = 0xa55a;
    return 0;
}
