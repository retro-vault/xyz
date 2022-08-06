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
S_SRCS	=	$(filter-out startup/$(CRT0).s, $(wildcard *.s) $(wildcard */*.s))
OBJS	=	$(addprefix $(BUILD_DIR)/,$(notdir $(CRT0).rel \
				$(patsubst %.c,%.rel,$(C_SRCS)) \
				$(patsubst %.s,%.rel,$(S_SRCS)) ) )
C8FLAGS	+=	-I$(ROOT)/src/yos
TARGET	=	yos

# Rules.
.PHONY: rom
rom: $(BIN_DIR)/$(TARGET).rom md

$(BIN_DIR)/$(TARGET).rom: $(BUILD_DIR)/$(TARGET).ihx
	$(OBJCOPY) -I ihex -O binary $(basename $<).ihx $(basename $@).rom
	$(TRUNC) --size 16K $(basename $@).rom

$(BUILD_DIR)/$(TARGET).ihx: $(OBJS)
	$(LD8) $(LD8FLAGS) -o $(BUILD_DIR)/$(TARGET).ihx $(OBJS)

$(BUILD_DIR)/$(CRT0).rel: startup/$(CRT0).s
	$(AS8) $(AS8FLAGS) -o $(BUILD_DIR)/$(CRT0).rel startup/$(CRT0).s

$(BUILD_DIR)/%.rel:	%.s
	$(AS8) $(AS8FLAGS) $(BUILD_DIR)/$(@F) $<

$(BUILD_DIR)/%.rel: %.c
	$(C8C) -c -o $(BUILD_DIR)/$(@F) $< $(C8FLAGS)

$(BUILD_DIR)/%.rel: %.h

.PHONY: md
md: 
	pandoc -t gfm --template docs/00_template.gfm --toc -s --toc-depth=3 docs/*.md > README.md
	