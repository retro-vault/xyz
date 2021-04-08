# We only allow compilation on linux!
ifneq ($(shell uname), Linux)
$(error OS must be Linux!)
endif

# Check if all required tools are on the system.
REQUIRED = gcc
K := $(foreach exec,$(REQUIRED),\
    $(if $(shell which $(exec)),,$(error "$(exec) not found. Please install or add to path.")))

# Global settings: folders.
ROOT = $(realpath .)
export BUILD_DIR=	$(ROOT)/build
export BIN_DIR	=	$(ROOT)/bin
export INC_DIR	=	$(ROOT)/include
export SCR_DIR	=	$(ROOT)/scripts

# Globa settings: tools.
export CC		=	gcc
export CFLAGS	=	-w -lSDL2 -g -std=c99
export AR		=	ar 
export ARFLAGS	=	-crs

# Subfolders for make.
SUBDIRS =  src

# Rules.
.PHONY: all
all:	dirs $(SUBDIRS) copy

.PHONY: dirs
dirs:
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BIN_DIR)

.PHONY: $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@
	
.PHONY: copy
copy:
	cp $(BUILD_DIR)/zwin $(BIN_DIR)/zwin
	
.PHONY: clean
clean:
	rm -f -r $(BUILD_DIR)
	rm -f -r $(BIN_DIR)
