# ---------------------------------------------------------------------------
# src/xc/Makefile — builds compiler toolchain.
# xld: Z80 linker (uses libxbfd)
# xgdb: debugger stub
# xcc: C compiler
# xas: Z80 assembler (uses libxbfd)
# xar: archive tool (uses libxbfd)
# xobjcopy: object/archive conversion tool (uses libxbfd)
# xopt: standalone assembly optimizer (uses libxopt)
# ---------------------------------------------------------------------------

SUBDIRS := xld xgdb xcc xopt xas xar xobjcopy

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
