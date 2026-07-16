#include "xcc_exec_test.h"

static const unsigned char rle_data[] = {
    5u, 5u, 5u, 2u, 9u, 9u, 7u, 7u, 7u, 7u
};

static unsigned int
rle_checksum(void)
{
    const unsigned char *cursor = rle_data;
    unsigned int index = 0u;
    unsigned int checksum = 0u;

    while (index < sizeof(rle_data)) {
        unsigned char value = *cursor;
        unsigned char run = 1u;

        ++index;
        ++cursor;
        while (index < sizeof(rle_data) && *cursor == value) {
            ++run;
            ++index;
            ++cursor;
        }
        unsigned int packed =
            (unsigned int)value | ((unsigned int)run << 8);
        checksum = (unsigned int)(checksum * 17u + packed);
    }
    return checksum;
}

int
main(void)
{
    XCC_CHECK_EQ_UINT_ID(1, rle_checksum(), 15575u);
    return 0;
}
