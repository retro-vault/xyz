# Folders
SUBDIRS = xlink

# Rules.
.PHONY: tools
tools:	$(SUBDIRS) 

.PHONY: $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@
