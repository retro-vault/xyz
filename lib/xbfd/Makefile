ROOT ?= ../..
BUILD_DIR ?= $(ROOT)/build
DIST_DIR ?= $(ROOT)/bin
PUBLIC_LIB_DIR ?= $(DIST_DIR)/lib
CXX ?= g++
CXXFLAGS ?= -std=c++23 -Wall -Wextra -Wpedantic -g

INCLUDE  := -Iinclude -I.
OBJDIR   := $(BUILD_DIR)/lib/xbfd
BUILD_TARGET := $(BUILD_DIR)/lib/libxbfd.a
DIST_TARGET  := $(PUBLIC_LIB_DIR)/libxbfd.a

SOURCES  := \
    binary/elf_reader.cpp \
    binary/elf_writer.cpp \
    debug_info/cdb.cpp \
    debug_info/map_reader.cpp \
    debug_info/dwarf2_writer.cpp \
    debug_info/writer.cpp \
    object/bfd.cpp \
    object/rel_reader.cpp \
    object/rel_writer.cpp \
    object/ar_reader.cpp \
    object/ar_writer.cpp

OBJS := $(patsubst %.cpp,$(OBJDIR)/%.o,$(SOURCES))
DEPS := $(OBJS:.o=.d)

.PHONY: all clean

all: $(BUILD_TARGET) $(DIST_TARGET)

$(BUILD_TARGET): $(OBJS) | $(BUILD_DIR)/lib
	ar rcs $@ $^

$(DIST_TARGET): $(BUILD_TARGET) | $(PUBLIC_LIB_DIR)
	cp $< $@

$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -MMD -MP -c -o $@ $<

$(BUILD_DIR)/lib:
	mkdir -p $@

$(PUBLIC_LIB_DIR):
	mkdir -p $@

$(OBJDIR):
	mkdir -p $@

-include $(DEPS)

clean:
	rm -rf $(OBJDIR)
	rm -f $(BUILD_TARGET) $(DIST_TARGET)
