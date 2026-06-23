//
        // free_sized and free_aligned_sized are available in <stdlib.h>.
        // Generated from tests/spec/c23_suite.py.
        //
        // This file is part of the C23 compatibility suite.
        //

        #include <stdio.h>
#include <stdlib.h>


        int main(void)
        {
            void *plain = malloc(64);
            void *aligned = aligned_alloc(32, 64);

            if (plain == NULL || aligned == NULL)
                return 1;

            free_sized(plain, 64);
            free_aligned_sized(aligned, 32, 64);

            puts("OK free_sized_and_free_aligned_sized");
            return 0;
        }

