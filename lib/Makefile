# Host-side debug libraries.
SUBDIRS = xdbg xdbgstub

# Rules.
.PHONY: all
all:	$(SUBDIRS) 

.PHONY: $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@
