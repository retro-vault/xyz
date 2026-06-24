# Makefile — libc assembly function tests (gcc-comparison via Z80 emulation)
#
# Assembles the hand-written libc .s files (plus the runtime divide core that
# div() depends on), links them at 0x0000, then runs each function through the
# xz80 emulator and compares the result against the host (gcc) computation.

_THIS_DIR    := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
X_ROOT       ?= $(abspath $(_THIS_DIR)/../../..)
REPO_ROOT    ?= $(abspath $(X_ROOT)/..)
BUILD        := $(REPO_ROOT)/build/tests/libc
LIBC_SRC     := $(X_ROOT)/libc/src
RT_SRC       := $(X_ROOT)/runtime
XZ80_SRC     := $(X_ROOT)/lib/xz80/src
XZ80_INC     := $(X_ROOT)/lib/xz80/include
RT_TESTS     := $(X_ROOT)/tests/tests/runtime
LIBC_PROFILE ?= full
LIBC_FLOAT ?= 1
LIBC_DOUBLE ?= 1
LIBC_LONG ?= 1
LIBC_LONGLONG ?= 1
LIBC_BUILD_FLAVOR := $(LIBC_PROFILE)-f$(LIBC_FLOAT)-d$(LIBC_DOUBLE)-l$(LIBC_LONG)-ll$(LIBC_LONGLONG)
LIBC_ARCHIVE := $(BUILD)/sys-none/lib/libc/$(LIBC_BUILD_FLAVOR)/libc.a
LIBC_ARCHIVE_SRCS := $(sort \
  $(wildcard $(X_ROOT)/libc/src/*.c) \
  $(wildcard $(X_ROOT)/libc/src/*/*.c) \
  $(wildcard $(X_ROOT)/libc/src/*.s) \
  $(wildcard $(X_ROOT)/libc/src/*/*.s) \
  $(wildcard $(X_ROOT)/tests/tests/sysmock/*.c) \
  $(wildcard $(X_ROOT)/tests/tests/sysmock/*.s))

CXX         := g++
CXXFLAGS    := -std=c++20 -Wall -Wextra -g -I$(XZ80_INC) -I$(BUILD) -I$(RT_TESTS) -I.
C8C         ?= $(REPO_ROOT)/bin/x/bin/xcc
C8FLAGS     ?= -I$(X_ROOT)/libc/include

# The real libc/sys/runtime builds already assemble with xas, and the direct
# harness now follows that same default. Link with xld so the libc surface is
# exercised through the in-tree linker too.
ASM         ?= $(REPO_ROOT)/bin/x/bin/xas
ASMFLAGS    ?= --mode=sdcc
LINK        ?= $(REPO_ROOT)/bin/x/bin/xld
# Keep only text anchored for the host-driven libc images. Some split test
# images legitimately grow past 0xF000 before RAM areas begin, so forcing a
# fixed -Tdata there creates a false linker failure.
LINKFLAGS   ?= --mode=sdcc --oformat=ihx -Ttext=0x0000 -nostdlib

# Hand-written libc functions under test, plus the runtime signed-divide core
# that div() tail-calls.
SRCS := \
  $(LIBC_SRC)/stdlib/abs.s \
  $(LIBC_SRC)/stdlib/labs.s \
  $(LIBC_SRC)/stdlib/llabs.s \
  $(LIBC_SRC)/stdlib/div.s \
  $(LIBC_SRC)/stdlib/ldiv.s \
  $(LIBC_SRC)/stdlib/lldiv.s \
  $(LIBC_SRC)/stdlib/rand.s \
  $(LIBC_SRC)/stdlib/srand.s \
  $(LIBC_SRC)/stdlib/strtox_core.s \
  $(LIBC_SRC)/stdlib/strtol.s \
  $(LIBC_SRC)/stdlib/strtoul.s \
  $(LIBC_SRC)/stdlib/strtoll.s \
  $(LIBC_SRC)/stdlib/strtoull.s \
  $(LIBC_SRC)/stdlib/atoi.s \
  $(LIBC_SRC)/stdlib/atof.s \
  $(LIBC_SRC)/stdlib/atol.s \
  $(LIBC_SRC)/stdlib/atoll.s \
  $(LIBC_SRC)/stdlib/strtod_core.s \
  $(LIBC_SRC)/stdlib/strtof.s \
  $(LIBC_SRC)/stdlib/strtod.s \
  $(LIBC_SRC)/stdlib/strtold.s \
  $(LIBC_SRC)/stdlib/heap_core.s \
  $(LIBC_SRC)/stdlib/aligned_alloc.s \
  $(LIBC_SRC)/stdlib/bsearch.s \
  $(LIBC_SRC)/stdlib/qsort.s \
  $(LIBC_SRC)/stdlib/mblen.s \
  $(LIBC_SRC)/stdlib/mbtowc.s \
  $(LIBC_SRC)/stdlib/wctomb.s \
  $(LIBC_SRC)/stdlib/mbstowcs.s \
  $(LIBC_SRC)/stdlib/wcstombs.s \
  $(LIBC_SRC)/stdlib/getenv.s \
  $(LIBC_SRC)/stdlib/exit_core.s \
  $(LIBC_SRC)/stdlib/system.s \
  $(LIBC_SRC)/inttypes/imaxabs.s \
  $(LIBC_SRC)/string/string_common.s \
  $(LIBC_SRC)/string/stpcpy.s \
  $(LIBC_SRC)/string/stpncpy.s \
  $(LIBC_SRC)/string/mempcpy.s \
  $(LIBC_SRC)/string/memrchr.s \
  $(LIBC_SRC)/string/strchrnul.s \
  $(LIBC_SRC)/string/strcasecmp.s \
  $(LIBC_SRC)/string/strncasecmp.s \
  $(LIBC_SRC)/string/bzero.s \
  $(LIBC_SRC)/string/swab.s \
  $(LIBC_SRC)/string/strsep.s \
  $(LIBC_SRC)/string/strcasestr.s \
  $(LIBC_SRC)/string/rawmemchr.s \
  $(LIBC_SRC)/string/strlcpy.s \
  $(LIBC_SRC)/string/strlcat.s \
  $(LIBC_SRC)/string/index.s \
  $(LIBC_SRC)/string/rindex.s \
  $(LIBC_SRC)/string/bcopy.s \
  $(LIBC_SRC)/string/bcmp.s \
  $(LIBC_SRC)/string/strchr.s \
  $(LIBC_SRC)/string/strrchr.s \
  $(LIBC_SRC)/string/memchr.s \
  $(LIBC_SRC)/string/memcmp.s \
  $(LIBC_SRC)/string/memcpy.s \
  $(LIBC_SRC)/string/memmove.s \
  $(LIBC_SRC)/string/memset.s \
  $(LIBC_SRC)/string/strlen.s \
  $(LIBC_SRC)/string/strnlen.s \
  $(LIBC_SRC)/string/strcpy.s \
  $(LIBC_SRC)/string/strncpy.s \
  $(LIBC_SRC)/string/ffs.s \
  $(LIBC_SRC)/string/ffsl.s \
  $(LIBC_SRC)/string/ffsll.s \
  $(LIBC_SRC)/ctype/isascii.s \
  $(LIBC_SRC)/ctype/toascii.s \
  $(LIBC_SRC)/math/truncf.s \
  $(LIBC_SRC)/math/ldexpf.s \
  $(LIBC_SRC)/math/ilogbf.s \
  $(LIBC_SRC)/math/frexpf.s \
  $(LIBC_SRC)/math/atan2f.s \
  $(LIBC_SRC)/math/invtrigf.s \
  $(LIBC_SRC)/math/trigf.s \
  $(LIBC_SRC)/math/transf_core.s \
  $(LIBC_SRC)/math/expf.s \
  $(LIBC_SRC)/math/exp2f.s \
  $(LIBC_SRC)/math/expm1f.s \
  $(LIBC_SRC)/math/logf.s \
  $(LIBC_SRC)/math/log2f.s \
  $(LIBC_SRC)/math/log10f.s \
  $(LIBC_SRC)/math/log1pf.s \
  $(LIBC_SRC)/math/powf.s \
  $(LIBC_SRC)/math/cbrtf.s \
  $(LIBC_SRC)/math/fmaxf.s \
  $(LIBC_SRC)/math/fminf.s \
  $(LIBC_SRC)/math/nanf.s \
  $(LIBC_SRC)/math/significandf.s \
  $(LIBC_SRC)/math/round_common.s \
  $(LIBC_SRC)/math/floorf.s \
  $(LIBC_SRC)/math/ceilf.s \
  $(LIBC_SRC)/math/roundf.s \
  $(LIBC_SRC)/math/logbf.s \
  $(LIBC_SRC)/math/db1argf_common.s \
  $(LIBC_SRC)/math/legacymathd.s \
  $(LIBC_SRC)/math/moremathf.s \
  $(LIBC_SRC)/math/moremathd.s \
  $(LIBC_SRC)/math/fdimf.s \
  $(LIBC_SRC)/math/modff.s \
  $(LIBC_SRC)/time/time.s \
  $(LIBC_SRC)/time/clock.s \
  $(LIBC_SRC)/time/difftime.s \
  $(LIBC_SRC)/time/timespec_get.s \
  $(LIBC_SRC)/time/gmtime_r.s \
  $(LIBC_SRC)/time/gmtime.s \
  $(LIBC_SRC)/time/mktime.s \
  $(LIBC_SRC)/time/asctime_r.s \
  $(LIBC_SRC)/time/asctime.s \
  $(LIBC_SRC)/time/ctime_r.s \
  $(LIBC_SRC)/time/ctime.s \
  $(LIBC_SRC)/time/strftime.s \
  $(LIBC_SRC)/stdio/printf.s \
  $(LIBC_SRC)/stdio/scanf.s \
  $(LIBC_SRC)/stdio/fscanf.s \
  $(LIBC_SRC)/stdio/sscanf.s \
  $(LIBC_SRC)/stdio/vscanf.s \
  $(LIBC_SRC)/stdio/vfscanf.s \
  $(LIBC_SRC)/stdio/vsscanf.s \
  $(LIBC_SRC)/stdio/scanf_core.s \
  $(LIBC_SRC)/stdio/stdio_io.s \
  $(LIBC_SRC)/stdio/fgetpos.s \
  $(LIBC_SRC)/stdio/fsetpos.s \
  $(LIBC_SRC)/stdio/freopen.s \
  $(LIBC_SRC)/stdio/remove.s \
  $(LIBC_SRC)/stdio/rename.s \
  $(LIBC_SRC)/stdio/setbuf.s \
  $(LIBC_SRC)/stdio/setvbuf.s \
  $(LIBC_SRC)/stdio/tmpfile.s \
  $(LIBC_SRC)/stdio/tmpnam.s \
  $(LIBC_SRC)/stdio/perror.s \
  $(X_ROOT)/tests/tests/sysmock/fileio.s \
  $(X_ROOT)/tests/tests/sysmock/open.s \
  $(X_ROOT)/tests/tests/sysmock/close.s \
  $(X_ROOT)/tests/tests/sysmock/lseek.s \
  $(X_ROOT)/tests/tests/sysmock/read.s \
  $(X_ROOT)/tests/tests/sysmock/write.s \
  $(X_ROOT)/tests/tests/sysmock/_exit.s \
  $(X_ROOT)/tests/tests/sysmock/heap_region.s \
  $(X_ROOT)/tests/tests/sysmock/unlink.s \
  $(X_ROOT)/tests/tests/sysmock/rename.s \
  $(X_ROOT)/tests/tests/sysmock/sys_getchar.s \
  $(X_ROOT)/tests/tests/sysmock/sys_putchar.s \
  $(X_ROOT)/tests/tests/sysmock/gettimeofday.s \
  $(X_ROOT)/tests/tests/sysmock/settimeofday.s \
  $(RT_SRC)/int32/divslong.s \
  $(RT_SRC)/int32/modslong.s \
  $(RT_SRC)/int32/divulong.s \
  $(RT_SRC)/int32/modulong.s \
  $(RT_SRC)/int32/slong2fs.s \
  $(RT_SRC)/int32/ulong2fs.s \
  $(RT_SRC)/int32/fs2slong.s \
  $(RT_SRC)/int32/fs2u32mag.s \
  $(RT_SRC)/int32/mullong.s \
  $(RT_SRC)/int32/mulsint2slong.s \
  $(RT_SRC)/int32/muluint2slong.s \
  $(RT_SRC)/int16/sint2fs.s \
  $(RT_SRC)/int16/uint2fs.s \
  $(RT_SRC)/int16/divsigned.s \
  $(RT_SRC)/int16/divunsigned.s \
  $(RT_SRC)/float/fssub.s \
  $(RT_SRC)/float/fsadd.s \
  $(RT_SRC)/float/fsmul.s \
  $(RT_SRC)/float/fsdiv.s \
  $(RT_SRC)/float/fscmp.s \
  $(RT_SRC)/float/fpret.s \
  $(RT_SRC)/float/fpunpack.s \
  $(RT_SRC)/float/fpmant.s \
  $(RT_SRC)/float/fppack.s \
  $(RT_SRC)/float/fp_zero32.s \
  $(RT_SRC)/double/db2fs.s \
  $(RT_SRC)/double/dbadd.s \
  $(RT_SRC)/double/fs2db.s \
  $(RT_SRC)/double/db2ll.s \
  $(RT_SRC)/double/db2int.s \
  $(RT_SRC)/double/dbcmp.s \
  $(RT_SRC)/double/dbdiv.s \
  $(RT_SRC)/double/dbmul.s \
  $(RT_SRC)/double/dbneg.s \
  $(RT_SRC)/double/dbsub.s \
  $(RT_SRC)/double/sint2db.s \
  $(RT_SRC)/double/ull2db.s \
  $(RT_SRC)/double/db_zero.s \
  $(RT_SRC)/int64/divsll.s \
  $(RT_SRC)/int64/divull.s \
  $(RT_SRC)/int64/ll2uint.s \
  $(RT_SRC)/int64/modsll.s \
  $(RT_SRC)/int64/modull.s \
  $(RT_SRC)/int64/mulll.s \
  $(RT_SRC)/int64/sint2ll.s \
  $(RT_SRC)/int64/slong2ll.s \
  $(RT_SRC)/int64/uint2ll.s \
  $(RT_SRC)/int64/ulong2ll.s \
  $(RT_SRC)/int16/mulint.s \
  $(RT_SRC)/jumps/call_bc_runtime.s \
  $(LIBC_SRC)/errno/errno.s \
  $(LIBC_SRC)/string/strcmp.s \
  $(LIBC_SRC)/ctype/ctype_common.s \
  $(LIBC_SRC)/ctype/isalnum.s \
  $(LIBC_SRC)/ctype/isalpha.s \
  $(LIBC_SRC)/ctype/isblank.s \
  $(LIBC_SRC)/ctype/iscntrl.s \
  $(LIBC_SRC)/ctype/isdigit.s \
  $(LIBC_SRC)/ctype/isgraph.s \
  $(LIBC_SRC)/ctype/islower.s \
  $(LIBC_SRC)/ctype/isprint.s \
  $(LIBC_SRC)/ctype/ispunct.s \
  $(LIBC_SRC)/ctype/isspace.s \
  $(LIBC_SRC)/ctype/isupper.s \
  $(LIBC_SRC)/ctype/isxdigit.s \
  $(LIBC_SRC)/ctype/tolower.s \
  $(LIBC_SRC)/ctype/toupper.s \
  $(LIBC_SRC)/math/fabsf.s \
  $(LIBC_SRC)/math/copysignf.s \
  $(LIBC_SRC)/math/sqrtf.s \
  $(LIBC_SRC)/math/sinhf.s \
  $(LIBC_SRC)/math/coshf.s \
  $(LIBC_SRC)/math/tanhf.s \
  $(LIBC_SRC)/math/libc_fpclassifyf.s \
  $(LIBC_SRC)/math/libc_signbitf.s \
  $(LIBC_SRC)/math/libc_isnanf.s \
  $(LIBC_SRC)/math/libc_isinff.s \
  $(LIBC_SRC)/math/libc_isfinitef.s \
  $(LIBC_SRC)/fenv/fenv_common.s \
  $(LIBC_SRC)/fenv/feclearexcept.s \
  $(LIBC_SRC)/fenv/fegetexceptflag.s \
  $(LIBC_SRC)/fenv/feraiseexcept.s \
  $(LIBC_SRC)/fenv/fesetexceptflag.s \
  $(LIBC_SRC)/fenv/fetestexcept.s \
  $(LIBC_SRC)/fenv/fegetround.s \
  $(LIBC_SRC)/fenv/fesetround.s \
  $(LIBC_SRC)/fenv/fegetenv.s \
  $(LIBC_SRC)/fenv/feholdexcept.s \
  $(LIBC_SRC)/fenv/fesetenv.s \
  $(LIBC_SRC)/fenv/feupdateenv.s \
  $(LIBC_SRC)/locale/locale_common.s \
  $(LIBC_SRC)/locale/setlocale.s \
  $(LIBC_SRC)/locale/localeconv.s \
  $(LIBC_SRC)/uchar/mbrtoc16.s \
  $(LIBC_SRC)/uchar/c16rtomb.s \
  $(LIBC_SRC)/uchar/mbrtoc32.s \
  $(LIBC_SRC)/uchar/c32rtomb.s \
  $(LIBC_SRC)/signal/signal_common.s \
  $(LIBC_SRC)/signal/signal.s \
  $(LIBC_SRC)/wctype/towlower.s \
  $(LIBC_SRC)/wctype/towupper.s \
  $(LIBC_SRC)/wctype/iswalnum.s \
  $(LIBC_SRC)/wctype/iswalpha.s \
  $(LIBC_SRC)/wctype/iswblank.s \
  $(LIBC_SRC)/wctype/iswcntrl.s \
  $(LIBC_SRC)/wctype/iswdigit.s \
  $(LIBC_SRC)/wctype/iswgraph.s \
  $(LIBC_SRC)/wctype/iswlower.s \
  $(LIBC_SRC)/wctype/iswprint.s \
  $(LIBC_SRC)/wctype/iswpunct.s \
  $(LIBC_SRC)/wctype/iswspace.s \
  $(LIBC_SRC)/wctype/iswupper.s \
  $(LIBC_SRC)/wctype/iswxdigit.s \
  $(LIBC_SRC)/wctype/wctype.s \
  $(LIBC_SRC)/wctype/iswctype.s \
  $(LIBC_SRC)/wctype/wctrans.s \
  $(LIBC_SRC)/wctype/towctrans.s \
  $(LIBC_SRC)/wchar/wchar_common.s \
  $(LIBC_SRC)/wchar/btowc.s \
  $(LIBC_SRC)/wchar/wctob.s \
  $(LIBC_SRC)/wchar/mbsinit.s \
  $(LIBC_SRC)/wchar/wcslen.s \
  $(LIBC_SRC)/wchar/wcscpy.s \
  $(LIBC_SRC)/wchar/wcscat.s \
  $(LIBC_SRC)/wchar/wcschr.s \
  $(LIBC_SRC)/wchar/wcscmp.s \
  $(LIBC_SRC)/wchar/wcsncmp.s \
  $(LIBC_SRC)/wchar/wcsncpy.s \
  $(LIBC_SRC)/wchar/wcsnlen.s \
  $(LIBC_SRC)/wchar/wcsncat.s \
  $(LIBC_SRC)/wchar/wcscspn.s \
  $(LIBC_SRC)/wchar/wcsspn.s \
  $(LIBC_SRC)/wchar/wcspbrk.s \
  $(LIBC_SRC)/wchar/wcsrchr.s \
  $(LIBC_SRC)/wchar/wcsstr.s \
  $(LIBC_SRC)/wchar/wcstok.s \
  $(LIBC_SRC)/wchar/wmemchr.s \
  $(LIBC_SRC)/wchar/wmemcmp.s \
  $(LIBC_SRC)/wchar/wmemcpy.s \
  $(LIBC_SRC)/wchar/wmemmove.s \
  $(LIBC_SRC)/wchar/wmemset.s \
  $(LIBC_SRC)/assert/assert_fail.s \
  $(LIBC_SRC)/complex/cabsf.s \
  $(LIBC_SRC)/complex/cacosf.s \
  $(LIBC_SRC)/complex/cacoshf.s \
  $(LIBC_SRC)/complex/cargf.s \
  $(LIBC_SRC)/complex/casinf.s \
  $(LIBC_SRC)/complex/casinhf.s \
  $(LIBC_SRC)/complex/catanf.s \
  $(LIBC_SRC)/complex/catanhf.s \
  $(LIBC_SRC)/complex/ccosf.s \
  $(LIBC_SRC)/complex/ccoshf.s \
  $(LIBC_SRC)/complex/cexpf.s \
  $(LIBC_SRC)/complex/clogf.s \
  $(LIBC_SRC)/complex/cmplxf.s \
  $(LIBC_SRC)/complex/cpowf.s \
  $(LIBC_SRC)/complex/cprojf.s \
  $(LIBC_SRC)/complex/csinf.s \
  $(LIBC_SRC)/complex/csinhf.s \
  $(LIBC_SRC)/complex/csqrtf.s \
  $(LIBC_SRC)/complex/ctanf.s \
  $(LIBC_SRC)/complex/ctanhf.s \
  $(LIBC_SRC)/setjmp/setjmp.s \
  $(LIBC_SRC)/setjmp/longjmp.s \
  $(LIBC_SRC)/threads/call_once.s \
  $(LIBC_SRC)/threads/cnd_broadcast.s \
  $(LIBC_SRC)/threads/cnd_destroy.s \
  $(LIBC_SRC)/threads/cnd_init.s \
  $(LIBC_SRC)/threads/cnd_signal.s \
  $(LIBC_SRC)/threads/cnd_timedwait.s \
  $(LIBC_SRC)/threads/cnd_wait.s \
  $(LIBC_SRC)/threads/mtx_destroy.s \
  $(LIBC_SRC)/threads/mtx_init.s \
  $(LIBC_SRC)/threads/mtx_lock.s \
  $(LIBC_SRC)/threads/mtx_timedlock.s \
  $(LIBC_SRC)/threads/mtx_trylock.s \
  $(LIBC_SRC)/threads/mtx_unlock.s \
  $(LIBC_SRC)/threads/thrd_create.s \
  $(LIBC_SRC)/threads/thrd_current.s \
  $(LIBC_SRC)/threads/thrd_detach.s \
  $(LIBC_SRC)/threads/thrd_equal.s \
  $(LIBC_SRC)/threads/thrd_exit.s \
  $(LIBC_SRC)/threads/thrd_join.s \
  $(LIBC_SRC)/threads/thrd_sleep.s \
  $(LIBC_SRC)/threads/thrd_yield.s \
  $(LIBC_SRC)/threads/tss_create.s \
  $(LIBC_SRC)/threads/tss_delete.s \
  $(LIBC_SRC)/threads/tss_get.s \
  $(LIBC_SRC)/threads/tss_set.s \
  $(LIBC_SRC)/threads/threads_common.s \
  $(LIBC_SRC)/string/strcoll.s \
  $(LIBC_SRC)/string/strdup.s \
  $(LIBC_SRC)/string/strndup.s \
  $(LIBC_SRC)/string/strerror.s \
  $(LIBC_SRC)/string/strtok.s \
  $(LIBC_SRC)/string/strxfrm.s \
  $(LIBC_SRC)/string/memccpy.s \
  $(LIBC_SRC)/math/acoshf.s \
  $(LIBC_SRC)/math/asinhf.s \
  $(LIBC_SRC)/math/atanhf.s \
  $(LIBC_SRC)/math/cosh.s \
  $(LIBC_SRC)/math/sinh.s \
  $(LIBC_SRC)/math/tanh.s \
  $(LIBC_SRC)/math/erf.s \
  $(LIBC_SRC)/math/erff.s \
  $(LIBC_SRC)/math/erff_core.s \
  $(LIBC_SRC)/math/lgammaf.s \
  $(LIBC_SRC)/math/gammaf_core.s \
  $(LIBC_SRC)/math/tgammaf.s

RUNTIME_SRCS := $(filter $(RT_SRC)/%,$(SRCS))
SYSMOCK_SRCS := $(sort $(wildcard $(X_ROOT)/tests/tests/sysmock/*.s))
SYSMOCK_RELS := $(patsubst $(X_ROOT)/tests/tests/sysmock/%.s,$(BUILD)/objs/sysmock_%.rel,$(SYSMOCK_SRCS))
DIRECT_RELS := $(patsubst %.s,$(BUILD)/objs/%.rel,$(notdir $(RUNTIME_SRCS))) $(SYSMOCK_RELS)
CORE_RELS := $(DIRECT_RELS) $(BUILD)/objs/stdio_cases.rel $(BUILD)/objs/c23_cases.rel
CORE_RELS += $(BUILD)/objs/core_force_refs.rel
SCAN_RELS := $(DIRECT_RELS) $(BUILD)/objs/stdio_scan_cases.rel

CORE_PARTS := 1 2 3
CORE_CPP_1 := $(BUILD)/test_main_core_1.cpp
CORE_CPP_2 := $(BUILD)/test_main_core_2.cpp
CORE_CPP_3 := $(BUILD)/test_main_core_3.cpp
CORE_IHX_1 := $(BUILD)/libc_core_1.ihx
CORE_IHX_2 := $(BUILD)/libc_core_2.ihx
CORE_IHX_3 := $(BUILD)/libc_core_3.ihx
CORE_MAP_1 := $(BUILD)/libc_core_1.map
CORE_MAP_2 := $(BUILD)/libc_core_2.map
CORE_MAP_3 := $(BUILD)/libc_core_3.map
CORE_BIN_1 := $(BUILD)/libc_core_1.bin
CORE_BIN_2 := $(BUILD)/libc_core_2.bin
CORE_BIN_3 := $(BUILD)/libc_core_3.bin
CORE_HDR_1 := $(BUILD)/libc_core_symbols_1.hpp
CORE_HDR_2 := $(BUILD)/libc_core_symbols_2.hpp
CORE_HDR_3 := $(BUILD)/libc_core_symbols_3.hpp
CORE_TST_1 := $(BUILD)/libc_core_tests_1
CORE_TST_2 := $(BUILD)/libc_core_tests_2
CORE_TST_3 := $(BUILD)/libc_core_tests_3

SCAN_IHX := $(BUILD)/libc_scan.ihx
SCAN_MAP := $(BUILD)/libc_scan.map
SCAN_BIN := $(BUILD)/libc_scan.bin
SCAN_HDR := $(BUILD)/libc_scan_symbols.hpp
SCAN_TST := $(BUILD)/libc_scan_tests

WIDE_SRCS := \
  $(LIBC_SRC)/stdio/printf.s \
  $(LIBC_SRC)/stdio/stdio_io.s \
  $(LIBC_SRC)/stdio/tmpnam.s \
  $(LIBC_SRC)/stdlib/strtod_core.s \
  $(LIBC_SRC)/errno/errno.s \
  $(LIBC_SRC)/wchar/btowc.s \
  $(LIBC_SRC)/wchar/wctob.s \
  $(LIBC_SRC)/wchar/fgetwc.s \
  $(LIBC_SRC)/wchar/fgetws.s \
  $(LIBC_SRC)/wchar/fputwc.s \
  $(LIBC_SRC)/wchar/fputws.s \
  $(LIBC_SRC)/wchar/getwc.s \
  $(LIBC_SRC)/wchar/getwchar.s \
  $(LIBC_SRC)/wchar/putwc.s \
  $(LIBC_SRC)/wchar/putwchar.s \
  $(LIBC_SRC)/wchar/ungetwc.s \
  $(X_ROOT)/tests/tests/sysmock/fileio.s \
  $(X_ROOT)/tests/tests/sysmock/open.s \
  $(X_ROOT)/tests/tests/sysmock/close.s \
  $(X_ROOT)/tests/tests/sysmock/lseek.s \
  $(X_ROOT)/tests/tests/sysmock/read.s \
  $(X_ROOT)/tests/tests/sysmock/write.s \
  $(X_ROOT)/tests/tests/sysmock/unlink.s \
  $(X_ROOT)/tests/tests/sysmock/rename.s \
  $(X_ROOT)/tests/tests/sysmock/sys_getchar.s \
  $(X_ROOT)/tests/tests/sysmock/sys_putchar.s \
  $(RT_SRC)/int16/divunsigned.s \
  $(RT_SRC)/int32/divulong.s \
  $(RT_SRC)/int32/modulong.s \
  $(RT_SRC)/double/db2fs.s \
  $(RT_SRC)/double/dbadd.s \
  $(RT_SRC)/double/dbdiv.s \
  $(RT_SRC)/double/dbmul.s \
  $(RT_SRC)/double/dbneg.s \
  $(RT_SRC)/double/db_zero.s \
  $(RT_SRC)/double/ull2db.s \
  $(RT_SRC)/int64/divull.s \
  $(RT_SRC)/int64/modull.s

WIDE_RELS := $(DIRECT_RELS) $(BUILD)/objs/wide_stdio_wide_cases.rel

WIDE_IHX := $(BUILD)/libc_wide.ihx
WIDE_MAP := $(BUILD)/libc_wide.map
WIDE_BIN := $(BUILD)/libc_wide.bin
WIDE_HDR := $(BUILD)/libc_wide_symbols.hpp
WIDE_TST := $(BUILD)/libc_wide_tests

XZ80_SRCS := $(XZ80_SRC)/cpu.cpp $(XZ80_SRC)/memory.cpp $(XZ80_SRC)/disassembler.cpp

.PHONY: all test core-test core1-test core2-test core3-test scan-test wide-test clean
all: test

test: core-test scan-test wide-test

core-test: core1-test core2-test core3-test

core1-test: $(CORE_TST_1)
	$(CORE_TST_1) $(CORE_BIN_1)

core2-test: $(CORE_TST_2)
	$(CORE_TST_2) $(CORE_BIN_2)

core3-test: $(CORE_TST_3)
	$(CORE_TST_3) $(CORE_BIN_3)

scan-test: $(SCAN_TST)
	$(SCAN_TST) $(SCAN_BIN)

wide-test: $(WIDE_TST)
	$(WIDE_TST) $(WIDE_BIN)

$(CORE_TST_1): $(CORE_HDR_1) $(CORE_BIN_1) $(CORE_CPP_1) $(RT_TESTS)/runtime_machine.hpp
	$(CXX) $(CXXFLAGS) $(XZ80_SRCS) $(CORE_CPP_1) -o $@

$(CORE_TST_2): $(CORE_HDR_2) $(CORE_BIN_2) $(CORE_CPP_2) $(RT_TESTS)/runtime_machine.hpp
	$(CXX) $(CXXFLAGS) $(XZ80_SRCS) $(CORE_CPP_2) -o $@

$(CORE_TST_3): $(CORE_HDR_3) $(CORE_BIN_3) $(CORE_CPP_3) $(RT_TESTS)/runtime_machine.hpp
	$(CXX) $(CXXFLAGS) $(XZ80_SRCS) $(CORE_CPP_3) -o $@

$(SCAN_TST): $(SCAN_HDR) $(SCAN_BIN) test_main_scan.cpp $(RT_TESTS)/runtime_machine.hpp
	$(CXX) $(CXXFLAGS) $(XZ80_SRCS) test_main_scan.cpp -o $@

$(WIDE_TST): $(WIDE_HDR) $(WIDE_BIN) test_main_wide.cpp $(RT_TESTS)/runtime_machine.hpp
	$(CXX) $(CXXFLAGS) $(XZ80_SRCS) test_main_wide.cpp -o $@

$(CORE_HDR_1): $(CORE_IHX_1)
	python3 $(RT_TESTS)/tools/gen_symbols.py $(CORE_MAP_1) $@

$(CORE_HDR_2): $(CORE_IHX_2)
	python3 $(RT_TESTS)/tools/gen_symbols.py $(CORE_MAP_2) $@

$(CORE_HDR_3): $(CORE_IHX_3)
	python3 $(RT_TESTS)/tools/gen_symbols.py $(CORE_MAP_3) $@

$(SCAN_HDR): $(SCAN_IHX)
	python3 $(RT_TESTS)/tools/gen_symbols.py $(SCAN_MAP) $@

$(WIDE_HDR): $(WIDE_IHX)
	python3 $(RT_TESTS)/tools/gen_symbols.py $(WIDE_MAP) $@

$(CORE_BIN_1): $(CORE_IHX_1)
	python3 $(RT_TESTS)/tools/ihx2bin.py $< $@

$(CORE_BIN_2): $(CORE_IHX_2)
	python3 $(RT_TESTS)/tools/ihx2bin.py $< $@

$(CORE_BIN_3): $(CORE_IHX_3)
	python3 $(RT_TESTS)/tools/ihx2bin.py $< $@

$(SCAN_BIN): $(SCAN_IHX)
	python3 $(RT_TESTS)/tools/ihx2bin.py $< $@

$(WIDE_BIN): $(WIDE_IHX)
	python3 $(RT_TESTS)/tools/ihx2bin.py $< $@

$(LIBC_ARCHIVE): $(LIBC_ARCHIVE_SRCS)
	$(MAKE) -C $(X_ROOT)/libc/src ROOT=$(abspath $(X_ROOT)) BUILD_DIR=$(abspath $(BUILD))/sys-none DIST_DIR=$(REPO_ROOT)/bin/x HOST_BIN_DIR=$(REPO_ROOT)/bin/x/bin SYS=none LIBC_PROFILE=$(LIBC_PROFILE) LIBC_FLOAT=$(LIBC_FLOAT) LIBC_DOUBLE=$(LIBC_DOUBLE) LIBC_LONG=$(LIBC_LONG) LIBC_LONGLONG=$(LIBC_LONGLONG) AR8FLAGS="--mode=gnu rcs" -j1

$(CORE_IHX_1): $(DIRECT_RELS) $(BUILD)/objs/stdio_cases.rel $(BUILD)/objs/c23_cases.rel $(BUILD)/objs/core_force_refs_1.rel $(LIBC_ARCHIVE)
	$(LINK) $(LINKFLAGS) -Map=$(CORE_MAP_1) -o $@ $(DIRECT_RELS) $(BUILD)/objs/stdio_cases.rel $(BUILD)/objs/c23_cases.rel $(BUILD)/objs/core_force_refs_1.rel $(LIBC_ARCHIVE)

$(CORE_IHX_2): $(DIRECT_RELS) $(BUILD)/objs/stdio_cases.rel $(BUILD)/objs/c23_cases.rel $(BUILD)/objs/core_force_refs_2.rel $(LIBC_ARCHIVE)
	$(LINK) $(LINKFLAGS) -Map=$(CORE_MAP_2) -o $@ $(DIRECT_RELS) $(BUILD)/objs/stdio_cases.rel $(BUILD)/objs/c23_cases.rel $(BUILD)/objs/core_force_refs_2.rel $(LIBC_ARCHIVE)

$(CORE_IHX_3): $(DIRECT_RELS) $(BUILD)/objs/stdio_cases.rel $(BUILD)/objs/c23_cases.rel $(BUILD)/objs/core_force_refs_3.rel $(LIBC_ARCHIVE)
	$(LINK) $(LINKFLAGS) -Map=$(CORE_MAP_3) -o $@ $(DIRECT_RELS) $(BUILD)/objs/stdio_cases.rel $(BUILD)/objs/c23_cases.rel $(BUILD)/objs/core_force_refs_3.rel $(LIBC_ARCHIVE)

$(SCAN_IHX): $(SCAN_RELS) $(LIBC_ARCHIVE)
	$(LINK) $(LINKFLAGS) -Map=$(SCAN_MAP) -o $@ $(SCAN_RELS) $(LIBC_ARCHIVE)

$(WIDE_IHX): $(WIDE_RELS) $(LIBC_ARCHIVE)
	$(LINK) $(LINKFLAGS) -Map=$(WIDE_MAP) -o $@ $(WIDE_RELS) $(LIBC_ARCHIVE)

# Pattern rules: each .rel comes from one of the source directories.
$(BUILD)/objs/%.rel: $(LIBC_SRC)/stdlib/%.s | $(BUILD)/objs
	$(ASM) $(ASMFLAGS) -o $@ $<
$(BUILD)/objs/%.rel: $(LIBC_SRC)/inttypes/%.s | $(BUILD)/objs
	$(ASM) $(ASMFLAGS) -o $@ $<
$(BUILD)/objs/%.rel: $(LIBC_SRC)/string/%.s | $(BUILD)/objs
	$(ASM) $(ASMFLAGS) -o $@ $<
$(BUILD)/objs/%.rel: $(LIBC_SRC)/ctype/%.s | $(BUILD)/objs
	$(ASM) $(ASMFLAGS) -o $@ $<
$(BUILD)/objs/%.rel: $(LIBC_SRC)/time/%.s | $(BUILD)/objs
	$(ASM) $(ASMFLAGS) -o $@ $<
$(BUILD)/objs/%.rel: $(LIBC_SRC)/math/%.s | $(BUILD)/objs
	$(ASM) $(ASMFLAGS) -o $@ $<
$(BUILD)/objs/%.rel: $(LIBC_SRC)/stdio/%.s | $(BUILD)/objs
	$(ASM) $(ASMFLAGS) -o $@ $<
$(BUILD)/objs/%.rel: $(X_ROOT)/tests/tests/sysmock/%.s | $(BUILD)/objs
	$(ASM) $(ASMFLAGS) -o $@ $<
$(BUILD)/objs/sysmock_%.rel: $(X_ROOT)/tests/tests/sysmock/%.s | $(BUILD)/objs
	$(ASM) $(ASMFLAGS) -o $@ $<
$(BUILD)/objs/%.rel: $(RT_SRC)/int32/%.s | $(BUILD)/objs
	$(ASM) $(ASMFLAGS) -o $@ $<
$(BUILD)/objs/%.rel: $(RT_SRC)/int16/%.s | $(BUILD)/objs
	$(ASM) $(ASMFLAGS) -o $@ $<
$(BUILD)/objs/%.rel: $(LIBC_SRC)/complex/%.s | $(BUILD)/objs
	$(ASM) $(ASMFLAGS) -o $@ $<
$(BUILD)/objs/%.rel: $(RT_SRC)/float/%.s | $(BUILD)/objs
	$(ASM) $(ASMFLAGS) -o $@ $<
$(BUILD)/objs/%.rel: $(RT_SRC)/double/%.s | $(BUILD)/objs
	$(ASM) $(ASMFLAGS) -o $@ $<
$(BUILD)/objs/%.rel: $(RT_SRC)/int64/%.s | $(BUILD)/objs
	$(ASM) $(ASMFLAGS) -o $@ $<
$(BUILD)/objs/%.rel: $(RT_SRC)/jumps/%.s | $(BUILD)/objs
	$(ASM) $(ASMFLAGS) -o $@ $<
$(BUILD)/objs/%.rel: $(LIBC_SRC)/errno/%.s | $(BUILD)/objs
	$(ASM) $(ASMFLAGS) -o $@ $<
$(BUILD)/objs/%.rel: $(LIBC_SRC)/fenv/%.s | $(BUILD)/objs
	$(ASM) $(ASMFLAGS) -o $@ $<
$(BUILD)/objs/%.rel: $(LIBC_SRC)/locale/%.s | $(BUILD)/objs
	$(ASM) $(ASMFLAGS) -o $@ $<
$(BUILD)/objs/%.rel: $(LIBC_SRC)/uchar/%.s | $(BUILD)/objs
	$(ASM) $(ASMFLAGS) -o $@ $<
$(BUILD)/objs/%.rel: $(LIBC_SRC)/signal/%.s | $(BUILD)/objs
	$(ASM) $(ASMFLAGS) -o $@ $<
$(BUILD)/objs/%.rel: $(LIBC_SRC)/wctype/%.s | $(BUILD)/objs
	$(ASM) $(ASMFLAGS) -o $@ $<
$(BUILD)/objs/%.rel: $(LIBC_SRC)/wchar/%.s | $(BUILD)/objs
	$(ASM) $(ASMFLAGS) -o $@ $<
$(BUILD)/objs/%.rel: $(LIBC_SRC)/assert/%.s | $(BUILD)/objs
	$(ASM) $(ASMFLAGS) -o $@ $<
$(BUILD)/objs/%.rel: $(LIBC_SRC)/setjmp/%.s | $(BUILD)/objs
	$(ASM) $(ASMFLAGS) -o $@ $<
$(BUILD)/objs/%.rel: $(LIBC_SRC)/threads/%.s | $(BUILD)/objs
	$(ASM) $(ASMFLAGS) -o $@ $<
$(BUILD)/objs/%.rel: $(BUILD)/objs/%.s | $(BUILD)/objs
	$(ASM) -o $@ $<

$(BUILD)/objs/wide_%.rel: $(LIBC_SRC)/stdio/%.s | $(BUILD)/objs
	$(ASM) $(ASMFLAGS) -o $@ $<
$(BUILD)/objs/wide_%.rel: $(LIBC_SRC)/stdlib/%.s | $(BUILD)/objs
	$(ASM) $(ASMFLAGS) -o $@ $<
$(BUILD)/objs/wide_%.rel: $(LIBC_SRC)/errno/%.s | $(BUILD)/objs
	$(ASM) $(ASMFLAGS) -o $@ $<
$(BUILD)/objs/wide_%.rel: $(LIBC_SRC)/wchar/%.s | $(BUILD)/objs
	$(ASM) $(ASMFLAGS) -o $@ $<
$(BUILD)/objs/wide_%.rel: $(X_ROOT)/tests/tests/sysmock/%.s | $(BUILD)/objs
	$(ASM) $(ASMFLAGS) -o $@ $<
$(BUILD)/objs/wide_%.rel: $(RT_SRC)/int16/%.s | $(BUILD)/objs
	$(ASM) $(ASMFLAGS) -o $@ $<
$(BUILD)/objs/wide_%.rel: $(RT_SRC)/int32/%.s | $(BUILD)/objs
	$(ASM) $(ASMFLAGS) -o $@ $<
$(BUILD)/objs/wide_%.rel: $(RT_SRC)/double/%.s | $(BUILD)/objs
	$(ASM) $(ASMFLAGS) -o $@ $<
$(BUILD)/objs/wide_%.rel: $(RT_SRC)/int64/%.s | $(BUILD)/objs
	$(ASM) $(ASMFLAGS) -o $@ $<
$(BUILD)/objs/wide_%.rel: $(BUILD)/objs/wide_%.s | $(BUILD)/objs
	$(ASM) -o $@ $<

$(BUILD)/objs/stdio_cases.s: stdio_cases.c | $(BUILD)/objs
	$(C8C) -S $(C8FLAGS) -o $@ $<

$(BUILD)/objs/c23_cases.s: c23_cases.c | $(BUILD)/objs
	$(C8C) -S $(C8FLAGS) -o $@ $<

$(CORE_CPP_1): test_main_core.cpp tools/split_core_tests.py | $(BUILD)/objs
	python3 tools/split_core_tests.py $< 3 1 $@

$(CORE_CPP_2): test_main_core.cpp tools/split_core_tests.py | $(BUILD)/objs
	python3 tools/split_core_tests.py $< 3 2 $@

$(CORE_CPP_3): test_main_core.cpp tools/split_core_tests.py | $(BUILD)/objs
	python3 tools/split_core_tests.py $< 3 3 $@

$(BUILD)/objs/core_force_refs_1.s: $(CORE_CPP_1) tools/gen_force_refs.py | $(BUILD)/objs
	python3 tools/gen_force_refs.py $< $@

$(BUILD)/objs/core_force_refs_2.s: $(CORE_CPP_2) tools/gen_force_refs.py | $(BUILD)/objs
	python3 tools/gen_force_refs.py $< $@

$(BUILD)/objs/core_force_refs_3.s: $(CORE_CPP_3) tools/gen_force_refs.py | $(BUILD)/objs
	python3 tools/gen_force_refs.py $< $@

# Also ensure it is part of the core image for the "another test" (compiler C23 focus)
# (already in CORE_RELS from previous edit)

$(BUILD)/objs/stdio_cases_dispatch.s: stdio_cases_dispatch.s | $(BUILD)/objs
	cp $< $@

$(BUILD)/objs/stdio_scan_cases.s: stdio_scan_cases.c | $(BUILD)/objs
	$(C8C) -S $(C8FLAGS) -o $@ $<

$(BUILD)/objs/wide_stdio_wide_cases.s: stdio_wide_cases.c | $(BUILD)/objs
	$(C8C) -S $(C8FLAGS) -o $@ $<

$(BUILD)/objs:
	mkdir -p $@

clean:
	rm -rf $(BUILD)
