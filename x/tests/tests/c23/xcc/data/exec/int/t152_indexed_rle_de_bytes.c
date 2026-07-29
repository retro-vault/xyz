#include "xcc_exec_test.h"

static unsigned char input_bytes[1024];
static unsigned char encoded_bytes[2048];

static unsigned int
capped_rle_encode(void)
{
    unsigned int input_index = 0;
    unsigned int output_index = 0;
    unsigned int run;
    unsigned char value;

    while (input_index < sizeof(input_bytes)) {
        value = input_bytes[input_index];
        run = 1;
        ++input_index;
        while (input_index < sizeof(input_bytes) &&
               input_bytes[input_index] == value &&
               run < 255) {
            ++run;
            ++input_index;
        }
        encoded_bytes[output_index++] = (unsigned char)run;
        encoded_bytes[output_index++] = value;
    }
    return output_index;
}

int
main(void)
{
    unsigned int i;
    unsigned int encoded_size;
    unsigned int checksum = 0;

    for (i = 0; i < sizeof(input_bytes); ++i)
        input_bytes[i] = 7;

    encoded_size = capped_rle_encode();
    for (i = 0; i < encoded_size; ++i)
        checksum += encoded_bytes[i];

    XCC_CHECK_EQ_UINT_ID(1, encoded_size, 10);
    XCC_CHECK_EQ_UINT_ID(2, encoded_bytes[0], 255);
    XCC_CHECK_EQ_UINT_ID(3, encoded_bytes[8], 4);
    XCC_CHECK_EQ_UINT_ID(4, encoded_bytes[9], 7);
    XCC_CHECK_EQ_UINT_ID(5, checksum, 1059);
    return 0;
}
