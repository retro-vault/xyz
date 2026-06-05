# ---------------------------------------------------------------------------
# src/xc/Makefile — builds compiler toolchain.
# xld: Z80 linker (uses libxbfd)
# xgdb: debugger stub
# xcc: C compiler
# xas: Z80 assembler (uses libxbfd)
# xar: archive tool (uses libxbfd)
# ---------------------------------------------------------------------------

SUBDIRS := xld xgdb xcc xas xar

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
