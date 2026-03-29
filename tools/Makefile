# ---------------------------------------------------------------------------
# tools/Makefile — delegates to individual tool sub-directories.
# Add new tool directories to SUBDIRS when they become ready to build.
# ---------------------------------------------------------------------------

ROOT ?= $(realpath ..)

SUBDIRS := serial

.PHONY: all
all:
	@for d in $(SUBDIRS); do \
	    echo "==> tools/$$d"; \
	    $(MAKE) -C $$d || exit 1; \
	done

.PHONY: clean
clean:
	@for d in $(SUBDIRS); do \
	    $(MAKE) -C $$d clean; \
	done
