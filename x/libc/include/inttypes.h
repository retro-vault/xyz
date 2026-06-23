/*
 * inttypes.h
 *
 * Standard C23 integer-format and max-width arithmetic support for the xcc
 * Z80 target.
 *
 * This header currently provides the target's fixed format/scan macros,
 * intmax helpers, and narrow string-to-intmax conversion entry points.
 * Wide-character conversion entry points will follow the wider wchar runtime.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef _INTTYPES_H
#define _INTTYPES_H

#define __STDC_VERSION_INTTYPES_H__ 202311L

#include <stdint.h>
#include <wchar.h>

typedef struct imaxdiv_t {
    intmax_t quot;
    intmax_t rem;
} imaxdiv_t;

/* fprintf / printf format macros. */
#define PRId8   "d"
#define PRIi8   "i"
#define PRIb8   "b"
#define PRIo8   "o"
#define PRIu8   "u"
#define PRIx8   "x"
#define PRIX8   "X"

#define PRId16  "d"
#define PRIi16  "i"
#define PRIb16  "b"
#define PRIo16  "o"
#define PRIu16  "u"
#define PRIx16  "x"
#define PRIX16  "X"

#define PRId32  "ld"
#define PRIi32  "li"
#define PRIb32  "lb"
#define PRIo32  "lo"
#define PRIu32  "lu"
#define PRIx32  "lx"
#define PRIX32  "lX"

#define PRId64  "lld"
#define PRIi64  "lli"
#define PRIb64  "llb"
#define PRIo64  "llo"
#define PRIu64  "llu"
#define PRIx64  "llx"
#define PRIX64  "llX"

#define PRIdLEAST8   PRId8
#define PRIiLEAST8   PRIi8
#define PRIbLEAST8   PRIb8
#define PRIoLEAST8   PRIo8
#define PRIuLEAST8   PRIu8
#define PRIxLEAST8   PRIx8
#define PRIXLEAST8   PRIX8
#define PRIdLEAST16  PRId16
#define PRIiLEAST16  PRIi16
#define PRIbLEAST16  PRIb16
#define PRIoLEAST16  PRIo16
#define PRIuLEAST16  PRIu16
#define PRIxLEAST16  PRIx16
#define PRIXLEAST16  PRIX16
#define PRIdLEAST32  PRId32
#define PRIiLEAST32  PRIi32
#define PRIbLEAST32  PRIb32
#define PRIoLEAST32  PRIo32
#define PRIuLEAST32  PRIu32
#define PRIxLEAST32  PRIx32
#define PRIXLEAST32  PRIX32
#define PRIdLEAST64  PRId64
#define PRIiLEAST64  PRIi64
#define PRIbLEAST64  PRIb64
#define PRIoLEAST64  PRIo64
#define PRIuLEAST64  PRIu64
#define PRIxLEAST64  PRIx64
#define PRIXLEAST64  PRIX64

#define PRIdFAST8    PRId8
#define PRIiFAST8    PRIi8
#define PRIbFAST8    PRIb8
#define PRIoFAST8    PRIo8
#define PRIuFAST8    PRIu8
#define PRIxFAST8    PRIx8
#define PRIXFAST8    PRIX8
#define PRIdFAST16   PRId16
#define PRIiFAST16   PRIi16
#define PRIbFAST16   PRIb16
#define PRIoFAST16   PRIo16
#define PRIuFAST16   PRIu16
#define PRIxFAST16   PRIx16
#define PRIXFAST16   PRIX16
#define PRIdFAST32   PRId32
#define PRIiFAST32   PRIi32
#define PRIbFAST32   PRIb32
#define PRIoFAST32   PRIo32
#define PRIuFAST32   PRIu32
#define PRIxFAST32   PRIx32
#define PRIXFAST32   PRIX32
#define PRIdFAST64   PRId64
#define PRIiFAST64   PRIi64
#define PRIbFAST64   PRIb64
#define PRIoFAST64   PRIo64
#define PRIuFAST64   PRIu64
#define PRIxFAST64   PRIx64
#define PRIXFAST64   PRIX64

#define PRIdMAX "lld"
#define PRIiMAX "lli"
#define PRIbMAX "llb"
#define PRIoMAX "llo"
#define PRIuMAX "llu"
#define PRIxMAX "llx"
#define PRIXMAX "llX"

#define PRIdPTR "d"
#define PRIiPTR "i"
#define PRIbPTR "b"
#define PRIoPTR "o"
#define PRIuPTR "u"
#define PRIxPTR "x"
#define PRIXPTR "X"

/* fscanf / scanf format macros. */
#define SCNd8   "hhd"
#define SCNi8   "hhi"
#define SCNo8   "hho"
#define SCNu8   "hhu"
#define SCNx8   "hhx"

#define SCNd16  "hd"
#define SCNi16  "hi"
#define SCNo16  "ho"
#define SCNu16  "hu"
#define SCNx16  "hx"

#define SCNd32  "ld"
#define SCNi32  "li"
#define SCNo32  "lo"
#define SCNu32  "lu"
#define SCNx32  "lx"

#define SCNd64  "lld"
#define SCNi64  "lli"
#define SCNo64  "llo"
#define SCNu64  "llu"
#define SCNx64  "llx"

#define SCNdLEAST8   SCNd8
#define SCNiLEAST8   SCNi8
#define SCNoLEAST8   SCNo8
#define SCNuLEAST8   SCNu8
#define SCNxLEAST8   SCNx8
#define SCNdLEAST16  SCNd16
#define SCNiLEAST16  SCNi16
#define SCNoLEAST16  SCNo16
#define SCNuLEAST16  SCNu16
#define SCNxLEAST16  SCNx16
#define SCNdLEAST32  SCNd32
#define SCNiLEAST32  SCNi32
#define SCNoLEAST32  SCNo32
#define SCNuLEAST32  SCNu32
#define SCNxLEAST32  SCNx32
#define SCNdLEAST64  SCNd64
#define SCNiLEAST64  SCNi64
#define SCNoLEAST64  SCNo64
#define SCNuLEAST64  SCNu64
#define SCNxLEAST64  SCNx64

#define SCNdFAST8    SCNd8
#define SCNiFAST8    SCNi8
#define SCNoFAST8    SCNo8
#define SCNuFAST8    SCNu8
#define SCNxFAST8    SCNx8
#define SCNdFAST16   SCNd16
#define SCNiFAST16   SCNi16
#define SCNoFAST16   SCNo16
#define SCNuFAST16   SCNu16
#define SCNxFAST16   SCNx16
#define SCNdFAST32   SCNd32
#define SCNiFAST32   SCNi32
#define SCNoFAST32   SCNo32
#define SCNuFAST32   SCNu32
#define SCNxFAST32   SCNx32
#define SCNdFAST64   SCNd64
#define SCNiFAST64   SCNi64
#define SCNoFAST64   SCNo64
#define SCNuFAST64   SCNu64
#define SCNxFAST64   SCNx64

#define SCNdMAX "lld"
#define SCNiMAX "lli"
#define SCNoMAX "llo"
#define SCNuMAX "llu"
#define SCNxMAX "llx"

#define SCNdPTR "hd"
#define SCNiPTR "hi"
#define SCNoPTR "ho"
#define SCNuPTR "hu"
#define SCNxPTR "hx"

intmax_t  imaxabs(intmax_t j);
imaxdiv_t imaxdiv(intmax_t numer, intmax_t denom);
intmax_t  strtoimax(const char *restrict nptr, char **restrict endptr, int base);
uintmax_t strtoumax(const char *restrict nptr, char **restrict endptr, int base);
intmax_t  wcstoimax(const wchar_t *restrict nptr, wchar_t **restrict endptr, int base);
uintmax_t wcstoumax(const wchar_t *restrict nptr, wchar_t **restrict endptr, int base);

#endif /* _INTTYPES_H */
