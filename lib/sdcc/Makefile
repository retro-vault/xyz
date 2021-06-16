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
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DIR)/%.rel:	%.s
	$(AS) $(ASFLAGS) $(BUILD_DIR)/$(basename $*).rel $*.s

$(BUILD_DIR)/%.rel: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILD_DIR)/%.rel: %.h