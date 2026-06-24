#include "xcc_exec_test.h"
#include <errno.h>
#include <wchar.h>

int main(void) {
    mbstate_t st;
    wchar_t wide_buf[8];
    const char *src;
    const wchar_t *wsrc;
    char narrow_buf[8];
    size_t n;
    wchar_t wc;

    st.__unused = 0;
    XCC_CHECK_EQ_ULONG_ID(1, mbrlen("A", 1u, &st), 1u);
    XCC_CHECK_EQ_ULONG_ID(2, mbrlen("", 1u, &st), 0u);
    XCC_CHECK_EQ_ULONG_ID(3, mbrlen("A", 0u, &st), (size_t)-2);

    wc = 0;
    st.__unused = 0;
    XCC_CHECK_EQ_ULONG_ID(4, mbrtowc(&wc, "Z", 1u, &st), 1u);
    XCC_CHECK_EQ_INT_ID(5, (int)wc, 'Z');
    XCC_CHECK_EQ_ULONG_ID(6, wcrtomb(narrow_buf, (wchar_t)'Q', &st), 1u);
    XCC_CHECK_EQ_INT_ID(7, narrow_buf[0], 'Q');

    src = "abc";
    st.__unused = 0;
    n = mbsrtowcs(wide_buf, &src, 8u, &st);
    XCC_CHECK_EQ_ULONG_ID(8, n, 3u);
    XCC_CHECK_ID(9, src == 0);
    XCC_CHECK_EQ_INT_ID(10, (int)wide_buf[0], 'a');
    XCC_CHECK_EQ_INT_ID(11, (int)wide_buf[1], 'b');
    XCC_CHECK_EQ_INT_ID(12, (int)wide_buf[2], 'c');
    XCC_CHECK_EQ_INT_ID(13, (int)wide_buf[3], 0);

    src = "wxyz";
    st.__unused = 0;
    n = mbsrtowcs(wide_buf, &src, 2u, &st);
    XCC_CHECK_EQ_ULONG_ID(14, n, 2u);
    XCC_CHECK_ID(15, src != 0);
    XCC_CHECK_EQ_INT_ID(16, (int)(unsigned char)*src, 'y');
    XCC_CHECK_EQ_INT_ID(17, (int)wide_buf[0], 'w');
    XCC_CHECK_EQ_INT_ID(18, (int)wide_buf[1], 'x');

    wide_buf[0] = (wchar_t)'d';
    wide_buf[1] = (wchar_t)'o';
    wide_buf[2] = (wchar_t)'g';
    wide_buf[3] = 0;
    wsrc = wide_buf;
    st.__unused = 0;
    n = wcsrtombs(narrow_buf, &wsrc, sizeof(narrow_buf), &st);
    XCC_CHECK_EQ_ULONG_ID(19, n, 3u);
    XCC_CHECK_ID(20, wsrc == 0);
    XCC_CHECK_EQ_INT_ID(21, narrow_buf[0], 'd');
    XCC_CHECK_EQ_INT_ID(22, narrow_buf[1], 'o');
    XCC_CHECK_EQ_INT_ID(23, narrow_buf[2], 'g');
    XCC_CHECK_EQ_INT_ID(24, narrow_buf[3], 0);

    wide_buf[0] = (wchar_t)'a';
    wide_buf[1] = (wchar_t)'b';
    wide_buf[2] = (wchar_t)'c';
    wide_buf[3] = 0;
    wsrc = wide_buf;
    st.__unused = 0;
    n = wcsrtombs(narrow_buf, &wsrc, 2u, &st);
    XCC_CHECK_EQ_ULONG_ID(25, n, 2u);
    XCC_CHECK_ID(26, wsrc != 0);
    XCC_CHECK_EQ_INT_ID(27, (int)*wsrc, 'c');
    XCC_CHECK_EQ_INT_ID(28, narrow_buf[0], 'a');
    XCC_CHECK_EQ_INT_ID(29, narrow_buf[1], 'b');

    wide_buf[0] = (wchar_t)0x1234;
    wide_buf[1] = 0;
    wsrc = wide_buf;
    errno = 0;
    st.__unused = 0;
    n = wcsrtombs(narrow_buf, &wsrc, sizeof(narrow_buf), &st);
    XCC_CHECK_EQ_ULONG_ID(30, n, (size_t)-1);
    XCC_CHECK_EQ_INT_ID(31, errno, 84);
    XCC_CHECK_ID(32, wsrc == wide_buf);

    return 0;
}
