# Host-side libraries.  xbfd must build first (tools depend on it).
SUBDIRS = xbfd xdbg xdbgstub

# Rules.
.PHONY: all
all: $(SUBDIRS)

.PHONY: $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@

xdbg: xbfd
xdbgstub: xbfd

.PHONY: clean
clean:
	@for d in $(SUBDIRS); do $(MAKE) -C $$d clean; done
