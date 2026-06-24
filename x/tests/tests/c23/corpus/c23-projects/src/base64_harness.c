#include <string.h>

#include "base64.h"

int main(void) {
    unsigned char raw[9];
    unsigned char encoded[32];
    unsigned int enc_len;

    raw[0] = 'X';
    raw[1] = ' ';
    raw[2] = 'T';
    raw[3] = 'o';
    raw[4] = 'o';
    raw[5] = 'l';
    raw[6] = 's';
    raw[7] = '!';
    raw[8] = '!';

    if (b64e_size(9) != 12) return 1;
    if (b64d_size(12) != 9) return 2;

    enc_len = b64_encode(raw, 9, encoded);
    if (enc_len != 12) return 3;
    if (strcmp((const char *)encoded, "WCBUb29scyEh") != 0) return 4;

    if (b64_int('+') != 62 || b64_int('/') != 63 || b64_int('=') != 64) {
        return 7;
    }
    return 0;
}
