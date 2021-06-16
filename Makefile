# We only allow compilation on linux!
ifneq ($(shell uname), Linux)
$(error OS must be Linux!)
endif

# Check if all required tools are on the system.
REQUIRED = sdcc sdar sdasz80 sdldz80 sdobjcopy fuse sed truncate
K := $(foreach exec,$(REQUIRED),\
    $(if $(shell which $(exec)),,$(error "$(exec) not found. Please install or add to path.")))

# Global settings: folders.
ROOT = $(realpath .)
export BUILD_DIR	=	$(ROOT)/build
export BIN_DIR		=	$(ROOT)/bin
export INC_DIR		=	$(ROOT)/include 
export LIB_DIR		=	$(ROOT)/lib

# Globa settings: tools.
export CC			=	sdcc
export CFLAGS		=	--std-c11 -mz80 --no-std-crt0 --nostdinc --nostdlib --debug -I. -I$(INC_DIR) 
export AS			=	sdasz80
export ASFLAGS		=	-xlos -g
export AR			=	sdar
export ARFLAGS		=	-rc
export LD			=	sdcc
export LDFLAGS_ROM	=	-mz80 -Wl -y --code-loc 0x0000 --data-loc 0x5b00 \
						--no-std-crt0 --nostdlib --nostdinc \
						-L$(BUILD_DIR) -llibsdcc -p

# Subfolders for make.
SUBDIRS = lib src

# IHX (deliverables)
IHX		=	$(wildcard $(BUILD_DIR)/*.ihx)
BIN		=	$(patsubst %.ihx,%.bin,$(IHX))

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

# Make .BIN files (from ihx).
.PHONY: bin
bin:
	#$(BUILD_DIR)/load $(basename $@)
	#cpmcp -f idpfdd $(BUILD_DIR)/fddb.img $@ 0:$(@F)
	#cp $(BUILD_DIR)/crt0cpm.rel $(BIN_DIR)
	#cp $(BUILD_DIR)/*.lib $(BIN_DIR)
	#cp $(BUILD_DIR)/*.com $(BIN_DIR)
	#cp $(BUILD_DIR)/fddb.img $(BIN_DIR)

	
.PHONY: clean
clean:
	rm -f -r $(BIN_DIR)
	rm -f -r $(BUILD_DIR)

