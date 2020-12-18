# Toolchain is sdcc!
export CC 		= sdcc
export CFLAGS	= -mz80 --no-std-crt0
export AS		= sdasz80
export ASFLAGS	= -xlos -g
export LD		= sdcc
export LDFLAGS	= -mz80 --no-std-crt0 --nostdlib

# Targets are subdirs.
SUBDIRS = graphics

# Rules.
.PHONY: all $(SUBDIRS)

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@