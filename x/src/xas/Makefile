ROOT ?= ../..
BUILD_DIR ?= $(ROOT)/build
DIST_DIR ?= $(ROOT)/bin/x
HOST_BIN_DIR ?= $(DIST_DIR)/bin
PUBLIC_LIB_DIR ?= $(DIST_DIR)/lib
CXX ?= g++
CXXFLAGS ?= -std=c++2a -Wall -Wextra -Wpedantic -g
GIT_VERSION_TAG := $(shell git -C $(ROOT) describe --tags --abbrev=0 --match 'v[0-9]*' 2>/dev/null || true)
XTOOLS_VERSION ?= $(if $(filter v%,$(GIT_VERSION_TAG)),$(patsubst v%,%,$(GIT_VERSION_TAG)),0.1.0)
CXXFLAGS += -DXTOOLS_VERSION=\"$(XTOOLS_VERSION)\"

XBFD_DIR  := $(ROOT)/lib/xbfd
XBFD_INC  := $(XBFD_DIR)/include
XBFD_LIB  := $(BUILD_DIR)/lib/libxbfd.a
XZ80_DIR  := $(ROOT)/lib/xz80
XZ80_INC  := $(XZ80_DIR)/include
XZ80_LIB  := $(PUBLIC_LIB_DIR)/libxz80.a

INCLUDE   := -Iinclude -I$(XBFD_INC) -I$(XZ80_INC)

TARGET    := $(HOST_BIN_DIR)/xas
OBJDIR    := $(BUILD_DIR)/xc/xas/obj

SRCS := \
    src/main.cpp \
    src/cli.cpp \
    src/frontend/lexer.cpp \
    src/frontend/macro.cpp \
    src/frontend/macro_dialect_gnu.cpp \
    src/frontend/macro_dialect_sdas.cpp \
    src/frontend/parser.cpp \
    src/backend/codegen.cpp \
    src/backend/macro_translate.cpp \
    src/backend/source_emitter.cpp \
    src/backend/rel_emitter.cpp \
    src/backend/elf_emitter.cpp

OBJS := $(patsubst src/%.cpp,$(OBJDIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

.PHONY: all test test-libs clean

TEST_DIR := $(ROOT)/tests/tests/xas

all: $(XBFD_LIB) $(XZ80_LIB) $(TARGET)

$(XBFD_LIB):
	$(MAKE) -C $(XBFD_DIR) ROOT=$(ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(DIST_DIR) PUBLIC_LIB_DIR=$(PUBLIC_LIB_DIR)

$(XZ80_LIB):
	$(MAKE) -C $(XZ80_DIR) ROOT=$(ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(DIST_DIR) PUBLIC_LIB_DIR=$(PUBLIC_LIB_DIR)

$(TARGET): $(OBJS) $(XBFD_LIB) $(XZ80_LIB) | $(HOST_BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(XBFD_LIB) $(XZ80_LIB)

$(OBJDIR)/%.o: src/%.cpp | dirs
	$(CXX) $(CXXFLAGS) $(INCLUDE) -MMD -MP -c -o $@ $<

dirs:
	@mkdir -p $(sort $(dir $(OBJS)))

$(HOST_BIN_DIR):
	@mkdir -p $@

-include $(DEPS)

test: all
	XAS=$(abspath $(TARGET)) bash $(TEST_DIR)/format_test.sh
	XAS=$(abspath $(TARGET)) bash $(TEST_DIR)/include_test.sh
	XAS=$(abspath $(TARGET)) bash $(TEST_DIR)/directive_compat_test.sh
	XAS=$(abspath $(TARGET)) bash $(TEST_DIR)/macro_test.sh
	XAS=$(abspath $(TARGET)) bash $(TEST_DIR)/macro_convert_test.sh
	XAS=$(abspath $(TARGET)) bash $(TEST_DIR)/sdcc_compat_test.sh

test-libs: all
	bash $(TEST_DIR)/library_format_roundtrip_test.sh

clean:
	rm -rf $(OBJDIR)
	rm -f $(TARGET)
