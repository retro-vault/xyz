# Source files.
CRT0	=	crt0rom
C_SRCS	=	$(wildcard *.c)
S_SRCS	=	$(wildcard *.s)
OBJS	=	$(BUILD_DIR)/$(CRT0).rel \
				$(patsubst %.c,$(BUILD_DIR)/%.rel,$(C_SRCS)) \
				$(patsubst %.s,$(BUILD_DIR)/%.rel,$(S_SRCS))
TARGET	=	yos

# Rules.
.PHONY: rom
rom: $(BIN_DIR)/$(TARGET).rom

$(BIN_DIR)/$(TARGET).rom: $(BUILD_DIR)/$(TARGET).ihx
	$(OBJCOPY) -I ihex -O binary $(basename $<).ihx $(basename $@).rom
	$(TRUNC) --size 16K $(basename $@).rom

$(BUILD_DIR)/$(TARGET).ihx: $(OBJS)
	$(LD) $(LDFLAGS) -o $(BUILD_DIR)/$(TARGET).ihx $(OBJS)

$(BUILD_DIR)/$(CRT0).rel: $(LIB_DIR)/sys/$(CRT0).s
	$(AS) $(ASFLAGS) -o $(BUILD_DIR)/$(CRT0).rel $(LIB_DIR)/sys/$(CRT0).s

$(BUILD_DIR)/%.rel: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILD_DIR)/%.rel:	%.s
	$(AS) $(ASFLAGS) $(BUILD_DIR)/$(basename $*).rel $*.s

$(BUILD_DIR)/%.rel: %.h