# Makefile — libc assembly function tests (gcc-comparison via Z80 emulation)
#
# Assembles the hand-written libc .s files (plus the runtime divide core that
# div() depends on), links them at 0x0000, then runs each function through the
# xz80 emulator and compares the result against the host (gcc) computation.

ROOT        ?= ../..
BUILD       := $(ROOT)/build/tests/libc
LIBC_SRC    := $(ROOT)/lib/libc/src
RT_SRC      := $(ROOT)/src/xc/xcc/lib/runtime
XZ80_SRC    := $(ROOT)/lib/xz80/src
XZ80_INC    := $(ROOT)/lib/xz80/include
RT_TESTS    := $(ROOT)/tests/runtime

CXX         := g++
CXXFLAGS    := -std=c++20 -Wall -Wextra -g -I$(XZ80_INC) -I$(BUILD) -I$(RT_TESTS) -I.

ASM         := sdasz80
LINK        := sdldz80

# Hand-written libc functions under test, plus the runtime signed-divide core
# that div() tail-calls.
SRCS := \
  $(LIBC_SRC)/stdlib/abs.s \
  $(LIBC_SRC)/stdlib/labs.s \
  $(LIBC_SRC)/stdlib/llabs.s \
  $(LIBC_SRC)/stdlib/div.s \
  $(LIBC_SRC)/stdlib/rand.s \
  $(LIBC_SRC)/stdlib/srand.s \
  $(LIBC_SRC)/stdlib/strtox_core.s \
  $(LIBC_SRC)/stdlib/strtol.s \
  $(LIBC_SRC)/stdlib/strtoul.s \
  $(LIBC_SRC)/stdlib/strtoll.s \
  $(LIBC_SRC)/stdlib/strtoull.s \
  $(LIBC_SRC)/stdlib/atoi.s \
  $(LIBC_SRC)/stdlib/atol.s \
  $(LIBC_SRC)/stdlib/atoll.s \
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
  $(LIBC_SRC)/string/memcmp.s \
  $(LIBC_SRC)/string/memmove.s \
  $(LIBC_SRC)/string/ffs.s \
  $(LIBC_SRC)/string/ffsl.s \
  $(LIBC_SRC)/string/ffsll.s \
  $(LIBC_SRC)/ctype/isascii.s \
  $(LIBC_SRC)/ctype/toascii.s \
  $(LIBC_SRC)/math/truncf.s \
  $(LIBC_SRC)/math/ldexpf.s \
  $(LIBC_SRC)/math/ilogbf.s \
  $(LIBC_SRC)/math/frexpf.s \
  $(LIBC_SRC)/math/fmaxf.s \
  $(LIBC_SRC)/math/fminf.s \
  $(LIBC_SRC)/math/nanf.s \
  $(LIBC_SRC)/math/significandf.s \
  $(LIBC_SRC)/math/round_common.s \
  $(LIBC_SRC)/math/floorf.s \
  $(LIBC_SRC)/math/ceilf.s \
  $(LIBC_SRC)/math/roundf.s \
  $(LIBC_SRC)/math/logbf.s \
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
  $(ROOT)/lib/sys/none/sys_gettimeofday.s \
  $(ROOT)/lib/sys/none/sys_settimeofday.s \
  $(RT_SRC)/int32/divslong.s \
  $(RT_SRC)/int32/modslong.s \
  $(RT_SRC)/int32/divulong.s \
  $(RT_SRC)/int32/modulong.s \
  $(RT_SRC)/int32/slong2fs.s \
  $(RT_SRC)/int32/ulong2fs.s \
  $(RT_SRC)/int32/mullong.s \
  $(RT_SRC)/int16/sint2fs.s \
  $(RT_SRC)/int16/uint2fs.s \
  $(RT_SRC)/int16/divsigned.s \
  $(RT_SRC)/int16/divunsigned.s \
  $(RT_SRC)/float/fssub.s \
  $(RT_SRC)/float/fsadd.s \
  $(RT_SRC)/float/fsmul.s \
  $(RT_SRC)/float/fsdiv.s \
  $(RT_SRC)/float/fpunpack.s \
  $(RT_SRC)/float/fpmant.s \
  $(RT_SRC)/float/fppack.s \
  $(RT_SRC)/float/fp_zero32.s \
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
  $(LIBC_SRC)/math/atan2f.s \
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
  $(LIBC_SRC)/wchar/wmemset.s

RELS := $(patsubst %.s,$(BUILD)/objs/%.rel,$(notdir $(SRCS)))

IHX  := $(BUILD)/libc.ihx
MAP  := $(BUILD)/libc.map
BIN  := $(BUILD)/libc.bin
HDR  := $(BUILD)/libc_symbols.hpp
TST  := $(BUILD)/libc_tests
XZ80_SRCS := $(XZ80_SRC)/cpu.cpp $(XZ80_SRC)/memory.cpp $(XZ80_SRC)/disassembler.cpp

.PHONY: all test clean
all: test

test: $(TST)
	$(TST) $(BIN)

$(TST): $(HDR) $(BIN) test_main.cpp $(RT_TESTS)/runtime_machine.hpp
	$(CXX) $(CXXFLAGS) $(XZ80_SRCS) test_main.cpp -o $@

$(HDR): $(IHX)
	python3 $(RT_TESTS)/tools/gen_symbols.py $(MAP) $@

$(BIN): $(IHX)
	python3 $(RT_TESTS)/tools/ihx2bin.py $< $@

$(IHX): $(RELS)
	$(LINK) -n -m -j -i $@ -b _CODE=0x0000 $^ 2>/dev/null || \
	  $(LINK) -n -m -j -i $@ -b _CODE=0x0000 $^

# Pattern rules: each .rel comes from one of the source directories.
$(BUILD)/objs/%.rel: $(LIBC_SRC)/stdlib/%.s | $(BUILD)/objs
	$(ASM) -o $@ $<
$(BUILD)/objs/%.rel: $(LIBC_SRC)/inttypes/%.s | $(BUILD)/objs
	$(ASM) -o $@ $<
$(BUILD)/objs/%.rel: $(LIBC_SRC)/string/%.s | $(BUILD)/objs
	$(ASM) -o $@ $<
$(BUILD)/objs/%.rel: $(LIBC_SRC)/ctype/%.s | $(BUILD)/objs
	$(ASM) -o $@ $<
$(BUILD)/objs/%.rel: $(LIBC_SRC)/time/%.s | $(BUILD)/objs
	$(ASM) -o $@ $<
$(BUILD)/objs/%.rel: $(LIBC_SRC)/math/%.s | $(BUILD)/objs
	$(ASM) -o $@ $<
$(BUILD)/objs/%.rel: $(ROOT)/lib/sys/none/%.s | $(BUILD)/objs
	$(ASM) -o $@ $<
$(BUILD)/objs/%.rel: $(RT_SRC)/int32/%.s | $(BUILD)/objs
	$(ASM) -o $@ $<
$(BUILD)/objs/%.rel: $(RT_SRC)/int16/%.s | $(BUILD)/objs
	$(ASM) -o $@ $<
$(BUILD)/objs/%.rel: $(RT_SRC)/float/%.s | $(BUILD)/objs
	$(ASM) -o $@ $<
$(BUILD)/objs/%.rel: $(LIBC_SRC)/errno/%.s | $(BUILD)/objs
	$(ASM) -o $@ $<
$(BUILD)/objs/%.rel: $(LIBC_SRC)/fenv/%.s | $(BUILD)/objs
	$(ASM) -o $@ $<
$(BUILD)/objs/%.rel: $(LIBC_SRC)/locale/%.s | $(BUILD)/objs
	$(ASM) -o $@ $<
$(BUILD)/objs/%.rel: $(LIBC_SRC)/uchar/%.s | $(BUILD)/objs
	$(ASM) -o $@ $<
$(BUILD)/objs/%.rel: $(LIBC_SRC)/signal/%.s | $(BUILD)/objs
	$(ASM) -o $@ $<
$(BUILD)/objs/%.rel: $(LIBC_SRC)/wctype/%.s | $(BUILD)/objs
	$(ASM) -o $@ $<
$(BUILD)/objs/%.rel: $(LIBC_SRC)/wchar/%.s | $(BUILD)/objs
	$(ASM) -o $@ $<

$(BUILD)/objs:
	mkdir -p $@

clean:
	rm -rf $(BUILD)
