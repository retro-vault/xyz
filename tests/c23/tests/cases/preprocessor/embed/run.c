//
// The #embed directive and __has_embed operator can embed binary payloads.
// Generated from tests/spec/c23_suite.py.
//
// This file is part of the C23 compatibility suite.
//

#include <stdio.h>


#if __has_embed("embed_payload.txt") != __STDC_EMBED_FOUND__
#error The embed payload should be discoverable.
#endif

static const unsigned char payload[] = {
#embed "embed_payload.txt"
};

int main(void)
{
    if (sizeof(payload) != 4)
        return 1;

    if (payload[0] != 'C' || payload[1] != '2')
        return 1;

    if (payload[2] != '3' || payload[3] != '\n')
        return 1;

    puts("OK embed");
    return 0;
}

