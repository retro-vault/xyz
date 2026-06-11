//
        // Qualifier-preserving search functions retain const qualification in their return types.
        // Generated from tests/spec/c23_suite.py.
        //
        // This file is part of the C23 compatibility suite.
        //

        #include <stdio.h>
#include <string.h>
#include <wchar.h>


        int main(void)
        {
            const char *const_text = "abc";
            char text[] = "abc";

            static_assert(_Generic(strchr(const_text, 'b'), const char *: 1, default: 0));
            static_assert(_Generic(strchr(text, 'b'), char *: 1, default: 0));
            static_assert(_Generic(memchr(const_text, 'b', 3), const void *: 1, default: 0));
            static_assert(_Generic(wcschr(L"abc", L'b'), const wchar_t *: 1, default: 0));

            puts("OK qualifier_preserving_library_functions");
            return 0;
        }

