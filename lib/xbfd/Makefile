ROOT ?= ../..
BUILD_DIR ?= $(ROOT)/build
CXX ?= g++
CXXFLAGS ?= -std=c++2a -Wall -Wextra -Wpedantic -g

INCLUDE  := -Iinclude
OUTPUT   := $(BUILD_DIR)/lib
TARGET   := $(OUTPUT)/libxbfd.a

SOURCES  := \
    bfd.cpp \
    rel_reader.cpp \
    rel_writer.cpp \
    elf_reader.cpp \
    elf_writer.cpp \
    ar_reader.cpp \
    ar_writer.cpp

OBJDIR   := $(BUILD_DIR)/lib/xbfd
OBJS     := $(patsubst %.cpp,$(OBJDIR)/%.o,$(SOURCES))
DEPS     := $(OBJS:.o=.d)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS) | $(OUTPUT)
	ar rcs $@ $^

$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -MMD -MP -c -o $@ $<

$(OUTPUT):
	mkdir -p $@

$(OBJDIR):
	mkdir -p $@

-include $(DEPS)

clean:
	rm -rf $(OBJDIR)
	rm -f $(TARGET)
