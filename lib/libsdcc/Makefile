# Source files.
C_SRCS	=	$(wildcard *.c)
S_SRCS	=	$(wildcard *.s)
OBJS	=	$(patsubst %.c,$(BUILD_DIR)/%.rel,$(C_SRCS)) $(patsubst %.s,$(BUILD_DIR)/%.rel,$(S_SRCS))

# Target is libcpm.
TARGET = libsdcc

# Rules.
.PHONY: all
all:	$(BUILD_DIR)/$(TARGET).lib 

$(BUILD_DIR)/$(TARGET).lib: $(OBJS)
	$(AR8) $(ARFLAGS) $@ $^

$(BUILD_DIR)/%.rel:	%.s
	$(AS8) $(AS8FLAGS) $(BUILD_DIR)/$(basename $*).rel $*.s

$(BUILD_DIR)/%.rel: %.c
	$(C8C) -c -o $@ $< $(C8FLAGS)

$(BUILD_DIR)/%.rel: %.h