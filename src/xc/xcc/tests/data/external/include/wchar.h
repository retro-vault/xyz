#ifndef _WCHAR_H
#define _WCHAR_H

typedef int wchar_t;
typedef unsigned int wint_t;
typedef unsigned int size_t;
typedef struct mbstate_t {
    unsigned int __unused;
} mbstate_t;
#define WEOF ((wint_t)-1)
#define NULL ((void*)0)

int wprintf(const wchar_t *, ...);
int wputchar(wchar_t);
size_t mbrlen(const char *, size_t, mbstate_t *);
size_t mbrtowc(wchar_t *, const char *, size_t, mbstate_t *);
size_t wcrtomb(char *, wchar_t, mbstate_t *);
size_t mbsrtowcs(wchar_t *, const char **, size_t, mbstate_t *);
size_t wcsrtombs(char *, const wchar_t **, size_t, mbstate_t *);
size_t wcslen(const wchar_t *);
wchar_t *wcscpy(wchar_t *, const wchar_t *);
int wcscmp(const wchar_t *, const wchar_t *);
wchar_t *wcscat(wchar_t *, const wchar_t *);

#endif
