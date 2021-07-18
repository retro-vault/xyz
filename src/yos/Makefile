# Add subfolders.
vpath %.c kernel
vpath %.c drivers
vpath %.c tty
vpath %.c apps
vpath %.s kernel
vpath %.s drivers
vpath %.s tty
vpath %.s apps

# Source files.
CRT0	=	crt0rom
C_SRCS	=	$(wildcard *.c) $(wildcard */*.c)
S_SRCS	=	$(wildcard *.s) $(wildcard */*.s)
OBJS	=	$(addprefix $(BUILD_DIR)/,$(notdir $(CRT0).rel \
				$(patsubst %.c,%.rel,$(C_SRCS)) \
				$(patsubst %.s,%.rel,$(S_SRCS)) ) )
CFLAGS	+=	-I$(ROOT)/src/yos
TARGET	=	yos

# Rules.
.PHONY: rom
rom: $(BIN_DIR)/$(TARGET).rom

$(BIN_DIR)/$(TARGET).rom: $(BUILD_DIR)/$(TARGET).ihx
	$(OBJCOPY) -I ihex -O binary $(basename $<).ihx $(basename $@).rom
	$(TRUNC) --size 16K $(basename $@).rom

$(BUILD_DIR)/$(TARGET).ihx: $(OBJS)
	$(LD) $(LDFLAGS) -o $(BUILD_DIR)/$(TARGET).ihx $(OBJS)

$(BUILD_DIR)/$(CRT0).rel: startup/$(CRT0).s0
	$(AS) $(ASFLAGS) -o $(BUILD_DIR)/$(CRT0).rel startup/$(CRT0).s0

$(BUILD_DIR)/%.rel:	%.s
	$(AS) $(ASFLAGS) $(BUILD_DIR)/$(@F) $<

$(BUILD_DIR)/%.rel: %.c
	$(CC) -c -o $(BUILD_DIR)/$(@F) $< $(CFLAGS)

$(BUILD_DIR)/%.rel: %.h