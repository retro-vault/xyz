# ---------------------------------------------------------------------------
# src/xc/Makefile — builds compiler toolchain (linker and assembler support).
# xcc: C compiler (SDCC, external)
# xas: Assembler (SDCC as, external)
# xlink: XYZ linker (custom Z80 linker)
# ---------------------------------------------------------------------------

SUBDIRS := xlink

.PHONY: all
all:
	@for d in $(SUBDIRS); do \
	    echo "==> src/xc/$$d"; \
	    $(MAKE) -C $$d || exit 1; \
	done

.PHONY: clean
clean:
	@for d in $(SUBDIRS); do \
	    $(MAKE) -C $$d clean; \
	done
