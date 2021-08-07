# Folders
SUBDIRS = libsdcc-z80 libc libgpx

# Rules.
.PHONY: all
all:	$(SUBDIRS) 

.PHONY: $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@ BUILD_DIR=$(BUILD_DIR) BIN_DIR=$(BIN_DIR) PLATFORM=zxspec48
