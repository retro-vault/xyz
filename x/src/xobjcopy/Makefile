ROOT ?= ../..
BUILD_DIR ?= $(ROOT)/build
DIST_DIR ?= $(ROOT)/bin/x
HOST_BIN_DIR ?= $(DIST_DIR)/bin
CXX ?= g++
CXXFLAGS ?= -std=c++2a -Wall -Wextra -Wpedantic -g
GIT_VERSION_TAG := $(shell git -C $(ROOT) describe --tags --abbrev=0 --match 'v[0-9]*' 2>/dev/null || true)
XOBJCOPY_VERSION ?= $(if $(filter v%,$(GIT_VERSION_TAG)),$(patsubst v%,%,$(GIT_VERSION_TAG)),0.1.0)
CXXFLAGS += -DXOBJCOPY_VERSION=\"$(XOBJCOPY_VERSION)\"

XBFD_DIR  := $(ROOT)/lib/xbfd
XBFD_INC  := $(XBFD_DIR)/include
XBFD_LIB  := $(BUILD_DIR)/lib/libxbfd.a

INCLUDE   := -Iinclude -I$(XBFD_INC)

TARGET    := $(HOST_BIN_DIR)/xobjcopy
OBJDIR    := $(BUILD_DIR)/xc/xobjcopy/obj
TEST_BIN  := $(BUILD_DIR)/xc/xobjcopy/xobjcopy_tests
TEST_DIR  := $(ROOT)/tests/tests/xobjcopy

SRCS := \
    src/main.cpp \
    src/cli.cpp \
    src/operations.cpp

TEST_SRCS := \
    $(TEST_DIR)/test_main.cpp \
    src/cli.cpp \
    src/operations.cpp

OBJS := $(patsubst src/%.cpp,$(OBJDIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

.PHONY: all test clean

all: $(XBFD_LIB) $(TARGET)

$(XBFD_LIB):
	$(MAKE) -C $(XBFD_DIR)

$(TARGET): $(OBJS) $(XBFD_LIB) | $(HOST_BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(XBFD_LIB)

$(OBJDIR)/%.o: src/%.cpp | dirs
	$(CXX) $(CXXFLAGS) $(INCLUDE) -MMD -MP -c -o $@ $<

dirs:
	@mkdir -p $(sort $(dir $(OBJS)))
	@mkdir -p $(dir $(TEST_BIN))

$(HOST_BIN_DIR):
	@mkdir -p $@

test: $(XBFD_LIB) dirs $(TEST_SRCS)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -I$(TEST_DIR) -o $(TEST_BIN) $(TEST_SRCS) $(XBFD_LIB)
	$(TEST_BIN)

-include $(DEPS)

clean:
	rm -rf $(OBJDIR)
	rm -f $(TEST_BIN)
	rm -f $(TARGET)
