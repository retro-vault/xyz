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
            // C23 6.4.5p6: an L literal has unqualified wchar_t elements.
            static_assert(_Generic(wcschr(L"abc", L'b'), wchar_t *: 1, default: 0));
            const wchar_t wide_text[] = L"abc";
            static_assert(_Generic(wcschr(wide_text, L'b'), const wchar_t *: 1, default: 0));

            puts("OK qualifier_preserving_library_functions");
            return 0;
        }

