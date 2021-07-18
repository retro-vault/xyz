# We only allow compilation on linux!
ifneq ($(shell uname), Linux)
$(error OS must be Linux!)
endif

# Check if all required tools are on the system.
REQUIRED = sdcc sdar sdasz80 sdldz80 sdobjcopy fuse sed truncate
K := $(foreach exec,$(REQUIRED),\
    $(if $(shell which $(exec)),,$(error "$(exec) not found. Please install or add to path.")))

# Global settings: folders.
export ROOT = $(realpath .)
export BUILD_DIR	=	$(ROOT)/build
export BIN_DIR		=	$(ROOT)/bin
export INC_DIR		=	$(ROOT)/include
export STD_INC_DIR	=	$(ROOT)/lib/libc/include
export LIB_DIR		=	$(ROOT)/lib

# Globa settings: tools.
export CC			=	sdcc
export CFLAGS		=	--std-c11 -mz80 --debug \
						--no-std-crt0 --nostdinc --nostdlib \
						-I. -I$(STD_INC_DIR) -I$(INC_DIR) 
export AS			=	sdasz80
export ASFLAGS		=	-xlos -g
export AR			=	sdar
export ARFLAGS		=	-rc
export LD			=	sdcc
export LDFLAGS		=	-mz80 -Wl -y --code-loc 0x00ff --data-loc 0x5b00 \
						--no-std-crt0 --nostdlib --nostdinc \
						-L$(BUILD_DIR) -llibsdcc -llibc -p
export OBJCOPY		=	sdobjcopy
export TRUNC 		=	truncate

# Subfolders for make.
SUBDIRS = lib src

# Rules.
.PHONY: rom
rom:	mkdirs $(SUBDIRS)

.PHONY: mkdirs
mkdirs:
	# Create build dir.
	mkdir -p $(BUILD_DIR)
	# And bin dir.
	mkdir -p $(BIN_DIR)

.PHONY: $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@

.PHONY: clean
clean:
	rm -f -r $(BIN_DIR)
	rm -f -r $(BUILD_DIR)