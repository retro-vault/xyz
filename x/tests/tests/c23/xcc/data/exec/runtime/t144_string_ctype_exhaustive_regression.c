/*
 * Z80 C23 Exhaustive string.h + ctype.h Test
 *
 * Covers almost everything in <string.h> and <ctype.h> for C23.
 * Includes edge cases, overlapping memory, signed char handling,
 * locale-independent behavior (assumes "C" locale), and combined usage.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>
#include <stdbool.h>

static int tests_passed = 0;
static int tests_total = 0;

#define TEST_ASSERT(cond, msg) do { \
    tests_total++; \
    if (cond) { \
        tests_passed++; \
        printf("  [PASS] %s\n", msg); \
    } else { \
        printf("  [FAIL] %s\n", msg); \
    } \
} while(0)

int main(void) {
    printf("=== Z80 C23 STRING.H + CTYPE.H EXHAUSTIVE TEST ===\n\n");

    /* ===================== string.h ===================== */
    printf("--- string.h ---\n");

    /* strlen */
    TEST_ASSERT(strlen("") == 0, "strlen empty");
    TEST_ASSERT(strlen("hello") == 5, "strlen normal");
    TEST_ASSERT(strlen("a\0b") == 1, "strlen stops at null");

    /* strcpy / strncpy */
    {
        char buf[32];

        strcpy(buf, "test");
        TEST_ASSERT(strcmp(buf, "test") == 0, "strcpy");

        strncpy(buf, "abcde", 3);
        buf[3] = '\0';
        TEST_ASSERT(strcmp(buf, "abc") == 0, "strncpy limited");

        /* strcat / strncat */
        strcpy(buf, "hello");
        strcat(buf, " world");
        TEST_ASSERT(strcmp(buf, "hello world") == 0, "strcat");

        strcpy(buf, "hi");
        strncat(buf, " there!!!", 5);
        TEST_ASSERT(strcmp(buf, "hi ther") == 0, "strncat limited");
    }

    /* strcmp / strncmp */
    TEST_ASSERT(strcmp("abc", "abc") == 0, "strcmp equal");
    TEST_ASSERT(strcmp("abc", "abd") < 0, "strcmp less");
    TEST_ASSERT(strncmp("abcde", "abcfg", 3) == 0, "strncmp prefix");

    /* strchr / strrchr */
    {
        char *p = strchr("hello world", 'o');
        TEST_ASSERT(p != NULL && p[1] == ' ', "strchr finds first");
        p = strrchr("hello world", 'o');
        TEST_ASSERT(p != NULL && *(p + 1) == 'r', "strrchr finds last");
    }

    /* strstr */
    TEST_ASSERT(strstr("hello world", "world") != NULL, "strstr found");
    TEST_ASSERT(strstr("hello", "xyz") == NULL, "strstr not found");

    /* strspn / strcspn */
    TEST_ASSERT(strspn("123abc", "123") == 3, "strspn");
    TEST_ASSERT(strcspn("abc123", "123") == 3, "strcspn");

    /* strtok (careful - modifies string) */
    {
        char tokenbuf[] = "one,two,three";
        char *tok = strtok(tokenbuf, ",");
        TEST_ASSERT(tok != NULL && strcmp(tok, "one") == 0, "strtok first");
        tok = strtok(NULL, ",");
        TEST_ASSERT(tok != NULL && strcmp(tok, "two") == 0, "strtok second");
    }

    /* memcpy / memmove / memcmp */
    {
        unsigned char src[8] = {1, 2, 3, 4, 5, 6, 7, 8};
        unsigned char dst[8];
        unsigned char overlap[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

        memcpy(dst, src, 8);
        TEST_ASSERT(memcmp(dst, src, 8) == 0, "memcpy");

        /* overlapping memmove */
        memmove(overlap + 2, overlap, 5);
        TEST_ASSERT(overlap[2] == 0 && overlap[6] == 4, "memmove overlapping");

        memset(dst, 0xAA, 8);
        TEST_ASSERT(dst[0] == 0xAA && dst[7] == 0xAA, "memset");

        /* memchr */
        TEST_ASSERT(memchr(src, 5, 8) == &src[4], "memchr");
    }

    /* ===================== ctype.h ===================== */
    printf("\n--- ctype.h ---\n");

    /* is* functions */
    TEST_ASSERT(isalpha('A') && isalpha('z'), "isalpha");
    TEST_ASSERT(isdigit('0') && isdigit('9'), "isdigit");
    TEST_ASSERT(isalnum('A') && isalnum('5'), "isalnum");
    TEST_ASSERT(isspace(' ') && isspace('\t') && isspace('\n'), "isspace");
    TEST_ASSERT(ispunct('.') && ispunct('!'), "ispunct");
    TEST_ASSERT(isprint(' ') && isprint('~'), "isprint");
    TEST_ASSERT(isgraph('!') && isgraph('~'), "isgraph");
    TEST_ASSERT(iscntrl('\n') && iscntrl('\t'), "iscntrl");
    TEST_ASSERT(isxdigit('0') && isxdigit('F') && isxdigit('a'), "isxdigit");
    TEST_ASSERT(isblank(' ') && isblank('\t'), "isblank");
    TEST_ASSERT(islower('a') && islower('z'), "islower");
    TEST_ASSERT(isupper('A') && isupper('Z'), "isupper");

    /* tolower / toupper */
    TEST_ASSERT(tolower('A') == 'a', "tolower");
    TEST_ASSERT(toupper('z') == 'Z', "toupper");
    TEST_ASSERT(tolower('5') == '5', "tolower non-letter");
    TEST_ASSERT(toupper('!') == '!', "toupper non-letter");

    /* Edge: EOF handling (should not crash) */
    TEST_ASSERT(isalpha(EOF) == 0, "is* on EOF");
    TEST_ASSERT(tolower(EOF) == EOF, "tolower on EOF");

    /* Signed char handling (common Z80 issue) */
    {
        signed char sc = -1;   /* 0xFF */
        TEST_ASSERT(isalpha((unsigned char)sc) == 0, "isalpha on high bit set");
    }

    /* Combined usage example */
    {
        char mixed[] = "Hello, 123 World!";
        int letters = 0;
        int digits = 0;
        int spaces = 0;
        size_t i;

        for (i = 0; i < strlen(mixed); i++) {
            if (isalpha(mixed[i])) letters++;
            if (isdigit(mixed[i])) digits++;
            if (isspace(mixed[i])) spaces++;
        }
        TEST_ASSERT(letters == 10 && digits == 3 && spaces == 2,
                    "combined ctype + string");
    }

    printf("\n=== SUMMARY ===\n");
    printf("string.h + ctype.h tests passed: %d / %d\n", tests_passed, tests_total);
    if (tests_passed == tests_total) {
        printf("STRING + CTYPE TEST PASSED SUCCESSFULLY!\n");
    } else {
        printf("Some tests failed — check string/ctype implementation in your compiler.\n");
    }

    printf("\nNote: Behavior of some ctype functions with values >127 depends on locale and signedness.\n");
    printf("Most embedded/Z80 implementations use the \"C\" locale.\n");

    return 0;
}
