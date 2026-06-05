_THIS_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
ROOT ?= $(abspath $(_THIS_DIR)/../../..)
BUILD_DIR ?= $(ROOT)/build
DIST_DIR ?= $(ROOT)/bin
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

$(BUILD_DIR)/lib:
	mkdir -p $@

$(OBJDIR):
	mkdir -p $@

clean:
	rm -rf $(PROJECT_BUILD_DIR)
	rm -f $(BUILD_TARGET)
