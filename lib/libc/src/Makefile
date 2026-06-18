_THIS_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
ROOT ?= $(abspath $(_THIS_DIR)/../../..)
BUILD_DIR ?= $(ROOT)/build
DIST_DIR ?= $(ROOT)/bin/x
HOST_BIN_DIR ?= $(DIST_DIR)/bin

C8C ?= $(HOST_BIN_DIR)/xcc
AS8 ?= $(HOST_BIN_DIR)/xas
AR8 ?= $(HOST_BIN_DIR)/xar
C8FLAGS ?= -I$(ROOT)/lib/libc/include -I$(ROOT)/include
AS8FLAGS ?= --mode=sdcc
AR8FLAGS ?= rcs
LIBC_PROFILE ?= full
ifeq ($(LIBC_PROFILE),tiny)
override LIBC_FLOAT     := 0
override LIBC_DOUBLE    := 0
override LIBC_LONG      := 0
override LIBC_LONGLONG  := 0
else
LIBC_FLOAT     ?= 1
LIBC_DOUBLE    ?= 1
LIBC_LONG      ?= 1
LIBC_LONGLONG  ?= 1
endif

LIBC_FEATURE_DEFINES := \
	-D__XCC_LIBC_FLOAT=$(LIBC_FLOAT) \
	-D__XCC_LIBC_DOUBLE=$(LIBC_DOUBLE) \
	-D__XCC_LIBC_LONG=$(LIBC_LONG) \
	-D__XCC_LIBC_LONGLONG=$(LIBC_LONGLONG)
ifeq ($(LIBC_FLOAT),0)
LIBC_FEATURE_DEFINES += -D__XCC_LIBC_NO_FLOAT=1
endif
ifeq ($(LIBC_DOUBLE),0)
LIBC_FEATURE_DEFINES += -D__XCC_LIBC_NO_DOUBLE=1
endif
ifeq ($(LIBC_LONG),0)
LIBC_FEATURE_DEFINES += -D__XCC_LIBC_NO_LONG=1
endif
ifeq ($(LIBC_LONGLONG),0)
LIBC_FEATURE_DEFINES += -D__XCC_LIBC_NO_LONGLONG=1
endif
C8FLAGS += $(LIBC_FEATURE_DEFINES)
AS8FLAGS += $(LIBC_FEATURE_DEFINES)

LIBC_BUILD_FLAVOR := $(LIBC_PROFILE)-f$(LIBC_FLOAT)-d$(LIBC_DOUBLE)-l$(LIBC_LONG)-ll$(LIBC_LONGLONG)
PROJECT_BUILD_DIR := $(BUILD_DIR)/lib/libc/$(LIBC_BUILD_FLAVOR)
OBJDIR := $(PROJECT_BUILD_DIR)/obj
BUILD_TARGET := $(PROJECT_BUILD_DIR)/libc.a

C_SRCS := $(sort $(wildcard *.c */*.c))
S_SRCS := $(sort $(wildcard *.s */*.s))

ifeq ($(LIBC_FLOAT),0)
C_SRCS := $(filter-out complex/% fenv/% math/%,$(C_SRCS))
S_SRCS := $(filter-out complex/% fenv/% math/%,$(S_SRCS))
S_SRCS := $(filter-out \
	stdlib/atof.s \
	stdlib/sfp_mul10.s \
	stdlib/strfromd_core.s \
	stdlib/strtod_core.s \
	stdlib/strtof.s \
	time/difftime.s \
	wchar/wcstod_core.s \
	wchar/wcstof.s,$(S_SRCS))
endif
ifeq ($(LIBC_DOUBLE),0)
S_SRCS := $(filter-out \
	stdlib/atof.s \
	stdlib/strfroml.s \
	stdlib/strtod.s \
	stdlib/strtold.s \
	wchar/wcstod.s \
	wchar/wcstod_core.s \
	wchar/wcstof.s \
	wchar/wcstold.s,$(S_SRCS))
endif
ifeq ($(LIBC_LONG),0)
S_SRCS := $(filter-out \
	stdio/fgetpos.s \
	stdio/fseek.s \
	stdio/fsetpos.s \
	stdio/ftell.s \
	stdlib/atol.s \
	stdlib/ckd_mul_sint.s \
	stdlib/labs.s \
	stdlib/ldiv.s \
	stdlib/rand.s \
	stdlib/rand_state.s \
	stdlib/srand.s \
	stdlib/strtol.s \
	stdlib/strtoul.s \
	string/ffsl.s \
	time/% \
	wchar/wcstol.s \
	wchar/wcstoul.s,$(S_SRCS))
endif
ifeq ($(LIBC_LONGLONG),0)
S_SRCS := $(filter-out \
	inttypes/% \
	stdlib/atoll.s \
	stdlib/llabs.s \
	stdlib/lldiv.s \
	stdlib/strtoll.s \
	stdlib/strtoull.s \
	string/ffsll.s \
	wchar/wcstoll.s \
	wchar/wcstoull.s,$(S_SRCS))
endif

OBJS := $(patsubst %.c,$(OBJDIR)/%.rel,$(C_SRCS)) \
        $(patsubst %.s,$(OBJDIR)/%.rel,$(S_SRCS))

# Platform (sys) backend, selectable at build time.  "none" is the bare-metal
# default; an OS build sets SYS=<os> to pull in lib/sys/<os>/ instead.
WITH_SYS ?= 1
SYS ?= none
ifeq ($(WITH_SYS),1)
SYS_DIR  := $(ROOT)/lib/sys/$(SYS)
SYS_SRCS := $(sort $(filter-out $(SYS_DIR)/crt0.s,\
                    $(wildcard $(SYS_DIR)/*.s) $(wildcard $(SYS_DIR)/*.c)))
OBJS += $(patsubst $(SYS_DIR)/%.s,$(OBJDIR)/sys/%.rel,$(filter %.s,$(SYS_SRCS))) \
        $(patsubst $(SYS_DIR)/%.c,$(OBJDIR)/sys/%.rel,$(filter %.c,$(SYS_SRCS)))
endif

.PHONY: all clean

all: $(BUILD_TARGET)

$(BUILD_TARGET): $(OBJS) | $(PROJECT_BUILD_DIR)
	rm -f $@
	$(AR8) $(AR8FLAGS) $@ $^

$(OBJDIR)/%.s: %.c | $(OBJDIR)
	@mkdir -p $(dir $@)
	$(C8C) -S $(C8FLAGS) -o $@ $<

$(OBJDIR)/%.rel: %.s | $(OBJDIR)
	@mkdir -p $(dir $@)
	$(AS8) $(AS8FLAGS) $< -o $@

$(OBJDIR)/%.rel: $(OBJDIR)/%.s | $(OBJDIR)
	@mkdir -p $(dir $@)
	$(AS8) $(AS8FLAGS) $< -o $@

# sys backend objects (assembled / compiled from lib/sys/$(SYS)/)
ifeq ($(WITH_SYS),1)
$(OBJDIR)/sys/%.rel: $(SYS_DIR)/%.s | $(OBJDIR)
	@mkdir -p $(dir $@)
	$(AS8) $(AS8FLAGS) $< -o $@

$(OBJDIR)/sys/%.s: $(SYS_DIR)/%.c | $(OBJDIR)
	@mkdir -p $(dir $@)
	$(C8C) -S $(C8FLAGS) -o $@ $<

$(OBJDIR)/sys/%.rel: $(OBJDIR)/sys/%.s | $(OBJDIR)
	@mkdir -p $(dir $@)
	$(AS8) $(AS8FLAGS) $< -o $@
endif

$(PROJECT_BUILD_DIR):
	mkdir -p $@

$(OBJDIR): | $(PROJECT_BUILD_DIR)
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)/lib/libc
