# We only allow compilation on linux!
ifneq ($(shell uname), Linux)
$(error OS must be Linux!)
endif

# Check if all required tools are on the system.
REQUIRED = sdcc sdar sdasz80 sdldz80 sdobjcopy fuse sed truncate
K := $(foreach exec,$(REQUIRED),\
    $(if $(shell which $(exec)),,$(error "$(exec) not found. Please install or add to path.")))

# Global settings: folders.
export ROOT 		= $(realpath .)
export BUILD_DIR	=	$(ROOT)/build
export BIN_DIR		=	$(ROOT)/bin
export INC_DIR		=	$(ROOT)/include
export STD_INC_DIR	=	$(ROOT)/lib/libc/include
export LIB_DIR		=	$(ROOT)/lib

# Globa settings: 8 bit tools.
export C8C			=	sdcc
export C8FLAGS		=	--std-c11 -mz80 --debug \
						--no-std-crt0 --nostdinc --nostdlib \
						-I. -I$(STD_INC_DIR) -I$(INC_DIR) 
export AS8			=	sdasz80
export AS8FLAGS		=	-xlos -g
export AR8			=	sdar
export AR8FLAGS		=	-rc
export LD8			=	sdcc
export LD8FLAGS		=	-mz80 -Wl -y --code-loc 0x00ff --data-loc 0x5b00 \
						--no-std-crt0 --nostdlib --nostdinc \
						-L$(BUILD_DIR) -llibsdcc -llibc -p
export OBJCOPY		=	sdobjcopy
export TRUNC 		=	truncate

# Global settings: host tools.
export CXX			=	g++
export CXXFLAGS		=	-std=c++2a -I$(INC_DIR) -I$(LIB_DIR) -g

# Subfolders for make.
SUBDIRS8			=	lib src/yos
SUBDIRS_HOST		=	src/xc

# Rules.
.PHONY: all
all:	tools rom


.PHONY: mkdirs
mkdirs:
	# Create build dir.
	mkdir -p $(BUILD_DIR)
	# And bin dir.
	mkdir -p $(BIN_DIR)


.PHONY: rom
rom:	mkdirs $(SUBDIRS8)
.PHONY: $(SUBDIRS8)
$(SUBDIRS8):
	$(MAKE) -C $@


.PHONY: tools
tools:	mkdirs $(SUBDIRS_HOST)
.PHONY: $(SUBDIRS_HOST)
$(SUBDIRS_HOST):
	$(MAKE) -C $@


.PHONY: clean
clean:
	rm -f -r $(BIN_DIR)
	rm -f -r $(BUILD_DIR)