# Folders
SUBDIRS = yos

# Rules.
.PHONY: rom
rom:	$(SUBDIRS) 

.PHONY: $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@
