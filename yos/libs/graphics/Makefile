# Rules.
.PHONY: all

# Deps.
C_SRCS		=	$(wildcard *.c)
S_SRCS		=	$(wildcard *.s)
OBJS		=	$(patsubst %.c,$(BUILD_DIR)/%.rel,$(C_SRCS)) $(patsubst %.s,$(BUILD_DIR)/%.rel,$(S_SRCS))

all: $(BUILD_DIR)/graphics.lib

$(BUILD_DIR)/graphics.lib: $(OBJS)
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DIR)/%.rel: %.s
	$(AS) $(ASFLAGS) $(BUILD_DIR)/$(basename $*).rel $*.s

$(BUILD_DIR)/%.rel: %.c
	$(CC) -c -o $@ $< $(CFLAGS)