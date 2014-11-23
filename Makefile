ARCH = mips

export SYSROOT=/opt/clfs-mipsel-linux3.9.11-hard_float
export PKG_CONFIG=pkg-config-sysroot.sh
export PKG_CONFIG_LIBDIR=${SYSROOT}/usr/lib/pkgconfig

#replace the string for another.for example,use i386 to replace the i.386
HOSTARCH := $(shell uname -m | \
	sed -e s/i.86/i386/ \
	    -e s/sun4u/sparc64/ \
	    -e s/arm.*/arm/ \
	    -e s/sa110/arm/ \
	    -e s/powerpc/ppc/ \
	    -e s/macppc/ppc/)

HOSTOS := $(shell uname -s | tr '[:upper:]' '[:lower:]' | \
	    sed -e 's/\(cygwin\).*/cygwin/')

export	HOSTARCH HOSTOS ARCH

#########################################################################

TOPDIR	:= $(shell if [ "$$PWD" != "" ]; then echo $$PWD; else pwd; fi)
export	TOPDIR

LIBPATH := $(TOPDIR)/libs
export	LIBPATH

# load other configuration
include $(TOPDIR)/config.mk

ifndef CROSS_COMPILE

ifeq ($(ARCH),arm)
CROSS_COMPILE = arm-linux-
endif

ifeq ($(ARCH),i386)
ifeq ($(HOSTARCH),i386)
CROSS_COMPILE =
else
CROSS_COMPILE = i386-linux-
endif
endif

ifeq ($(ARCH),mips)
CROSS_COMPILE = mipsel-linux-
endif

ifeq ($(ARCH),nios)
CROSS_COMPILE = nios-elf-
endif

ifeq ($(ARCH),nios2)
CROSS_COMPILE = nios2-elf-
endif

ifeq ($(ARCH),m68k)
CROSS_COMPILE = m68k-elf-
endif

ifeq ($(ARCH),microblaze)
CROSS_COMPILE = mb-
endif

endif # ndef CROSS_COMPILE

export	CROSS_COMPILE

#########################################################################

# sub directories
SUBDIRS	= core apps/libinfo neux vprompt apps
.PHONY : $(SUBDIRS)

#########################################################################


all: depend $(SUBDIRS)

depend dep:
	@for dir in $(SUBDIRS) ; do $(MAKE) -C $$dir .depend ; done

$(SUBDIRS):
	$(MAKE) -C $@ all

clean:
	@for dir in $(SUBDIRS) ; do $(MAKE) -C $$dir clean ; done
