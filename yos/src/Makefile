# Toolchain is sdcc!
export CC 		= sdcc
export CFLAGS	= -mz80 --no-std-crt0
export AS		= sdasz80
export ASFLAGS	= -xlos -g
export LD		= sdcc
export LDFLAGS	= -o $(BUILD_DIR)/ -mz80 --no-std-crt0 --nostdlib -L $(BUILD_DIR) -l graphics
export AR		= sdar
export ARFLAGS	= -rc

# Rules.
.PHONY: all

# Deps.
C_SRCS		=	$(wildcard *.c)
S_SRCS		=	$(filter-out $(wildcard crt0*.s), $(wildcard *.s))
OBJS		=	$(patsubst %.c,$(BUILD_DIR)/%.rel,$(C_SRCS)) $(patsubst %.s,$(BUILD_DIR)/%.rel,$(S_SRCS))

all: $(BIN_DIR)/48.rom $(BIN_DIR)/yos.bin 
	
$(BIN_DIR)/48.rom: $(BUILD_DIR)/crt0-rom.rel $(OBJS)
	$(LD) $(LDFLAGS) $^
	$(BIN_DIR)/tools/makezxbin <$(BUILD_DIR)/crt0-rom.ihx >$(BIN_DIR)/48.rom
	truncate --size=16384 $(BIN_DIR)/48.rom

$(BIN_DIR)/yos.bin: $(BUILD_DIR)/crt0-ram.rel $(OBJS)
	$(info "RAM version: TODO!")

$(BUILD_DIR)/%.rel: %.s
	$(AS) $(ASFLAGS) $(BUILD_DIR)/$(basename $*).rel $*.s

$(BUILD_DIR)/%.rel: %.c
	$(CC) -c -o $@ $< $(CFLAGS)