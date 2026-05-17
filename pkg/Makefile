ROOT ?= $(realpath ..)

SUBDIRS := debian

.PHONY: all clean

all:
	@for d in $(SUBDIRS); do \
	    echo "==> pkg/$$d"; \
	    $(MAKE) -C $$d || exit 1; \
	done

clean:
	@for d in $(SUBDIRS); do \
	    $(MAKE) -C $$d clean; \
	done
