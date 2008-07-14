JUNK	= *~
MSRC=modules

KERNEL_PATH=/usr/src/linux

VERFILE := $(KERNEL_PATH)/include/linux/utsrelease.h
KERNELRELEASE := $(shell if [ -r $(VERFILE) ]; \
	then (cat $(VERFILE); echo UTS_RELEASE) | $(CC) $(INCLUDES) $(CFLAGS) -E - | tail -n 1 | xargs echo; \
	else uname -r; fi)
KERNELVER := $(shell echo "$(KERNELRELEASE)" | \
	sed "s/\([0-9]*\.[0-9]*\.[0-9]*\).*/\1/")
MODPATH := $(DESTDIR)/lib/modules/$(KERNELRELEASE)

all: modules

modules:
	$(MAKE) -C $(KERNEL_PATH) SUBDIRS=$(CURDIR)/modules MODVERDIR=$(CURDIR)/modules/build \
	BUILD_MATRIX=$(BUILD_MATRIX) \
	modules

modules_install:
	mkdir -p $(MODPATH)/kernel/drivers/sios
	cp $(MSRC)/*.ko $(MODPATH)/kernel/drivers/sios

uninstall:
	find $(MODPATH) -name "sios" | xargs rm -rf

clean:
	rm -rf $(MSRC)/build
	rm -f $(MSRC)/*.o
	rm -f $(MSRC)/*.ko
	rm -f $(MSRC)/.*[.k]o.cmd
	rm -f $(MSRC)/*.ver
	rm -f $(MSRC)/*.mod.[co]
	rm -f $(MSRC)/*.mod

.PHONY: modules modules_install uninstall clean