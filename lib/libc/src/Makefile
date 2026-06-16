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

PROJECT_BUILD_DIR := $(BUILD_DIR)/lib/libc
OBJDIR := $(PROJECT_BUILD_DIR)/obj
BUILD_TARGET := $(BUILD_DIR)/lib/libc.a

C_SRCS := $(sort $(wildcard *.c */*.c))
S_SRCS := $(sort $(wildcard *.s */*.s))
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

$(BUILD_TARGET): $(OBJS) | $(BUILD_DIR)/lib
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

$(BUILD_DIR)/lib:
	mkdir -p $@

$(OBJDIR):
	mkdir -p $@

clean:
	rm -rf $(PROJECT_BUILD_DIR)
	rm -f $(BUILD_TARGET)
