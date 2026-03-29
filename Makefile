# We only allow compilation on linux!
ifneq ($(shell uname), Linux)
$(error OS must be Linux!)
endif

# Check if docker is available.
K := $(if $(shell which docker),,$(error "docker not found. Please install Docker."))

# Global settings: folders.
export ROOT 		=	$(realpath .)
export BUILD_DIR	=	$(ROOT)/build
export BIN_DIR		=	$(ROOT)/bin
export INC_DIR		=	$(ROOT)/include \
						$(ROOT)/lib/libc/include \
						$(ROOT)/lib/libgpx/include
export LIB_DIR		=	$(ROOT)/lib

# Docker image for all 8-bit (Z80/ZX Spectrum) build tools.
# ROOT is mounted at the same host path so all absolute paths (BUILD_DIR,
# BIN_DIR, …) resolve identically inside the container.
# $$(pwd) defers to shell expansion in each recipe so the working directory
# matches the sub-make that invokes the tool.
DOCKER_IMAGE	=	wischner/sdcc-z80-zx-spectrum:latest
DOCKER_RUN		=	docker run --rm \
					-u $(shell id -u):$(shell id -g) \
					-v $(ROOT):$(ROOT) \
					-w $$(pwd) \
					$(DOCKER_IMAGE)

# 8-bit tools — all run inside the Docker container.
export C8C		=	$(DOCKER_RUN) sdcc
export C8FLAGS		=	--std-c11 -mz80 --debug \
						--nostdinc $(addprefix -I,$(INC_DIR))
export AS8		=	$(DOCKER_RUN) sdasz80
export AS8FLAGS		=	-xlos -g
export AR8		=	$(DOCKER_RUN) sdar
export AR8FLAGS		=	-rc
export LD8		=	$(DOCKER_RUN) sdcc
export LD8FLAGS		=	-mz80 -Wl -y --code-loc 0x00ff --data-loc 0x5b00 \
						--no-std-crt0 --nostdlib \
						-L$(BUILD_DIR) -llibgpx -llibsdcc-z80 -llibc -p
export OBJCOPY		=	$(DOCKER_RUN) sdobjcopy
export TRUNC		=	$(DOCKER_RUN) truncate

# Global settings: host tools.
export CXX			=	g++
export CXXFLAGS		=	-std=c++2a -I$(LIB_DIR) -g

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