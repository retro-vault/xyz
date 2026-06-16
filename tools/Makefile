# ---------------------------------------------------------------------------
# tools/Makefile — delegates to individual tool sub-directories.
# Add new tool directories to SUBDIRS when they become ready to build.
# ---------------------------------------------------------------------------

ROOT ?= $(realpath ..)
X_DIST_DIR ?= $(ROOT)/bin/x
Y_DIST_DIR ?= $(ROOT)/bin/y

Y_SUBDIRS := serial microdrive appmake
X_SUBDIRS := xgdb-vsix

.PHONY: all
all:
	@for d in $(Y_SUBDIRS); do \
	    echo "==> tools/$$d"; \
	    $(MAKE) -C $$d \
	        DIST_DIR=$(Y_DIST_DIR) \
	        HOST_BIN_DIR=$(Y_DIST_DIR)/bin \
	        PUBLIC_LIB_DIR=$(Y_DIST_DIR)/lib \
	        PUBLIC_INCLUDE_DIR=$(Y_DIST_DIR)/include || exit 1; \
	done
	@for d in $(X_SUBDIRS); do \
	    echo "==> tools/$$d"; \
	    $(MAKE) -C $$d \
	        DIST_DIR=$(X_DIST_DIR) \
	        HOST_BIN_DIR=$(X_DIST_DIR)/bin \
	        PUBLIC_LIB_DIR=$(X_DIST_DIR)/lib \
	        VSIX_STAGE_DIR=$(X_DIST_DIR)/pkg/vsix || exit 1; \
	done

.PHONY: clean
clean:
	@for d in $(Y_SUBDIRS) $(X_SUBDIRS); do \
	    $(MAKE) -C $$d clean; \
	done
