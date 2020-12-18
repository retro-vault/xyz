# Top configuration (OS dependent)
ifeq ($(OS),Windows_NT)
export BUILD_DIR	= ${CURDIR}/build
export BIN_DIR		= ${CURDIR}/bin
else ifeq ($(shell uname), Linux)
export BUILD_DIR	= $(shell pwd)/build
export BIN_DIR		= $(shell pwd)/bin
else
$(error OS must be Windows or Linux)
endif

# Targets are subdirs.
SUBDIRS = tools libs src

# Rules.
.PHONY: all clean $(SUBDIRS)

all: $(SUBDIRS)

clean: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)