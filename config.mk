
# clean the slate ...
PLATFORM_RELFLAGS =
PLATFORM_CPPFLAGS =
PLATFORM_LDFLAGS =

#
# When cross-compiling on NetBSD, we have to define __PPC__ or else we
# will pick up a va_list declaration that is incompatible with the
# actual argument lists emitted by the compiler.
#
# [Tested on NetBSD/i386 1.5 + cross-powerpc-netbsd-1.3]

ifeq ($(ARCH),ppc)
ifeq ($(CROSS_COMPILE),powerpc-netbsd-)
PLATFORM_CPPFLAGS+= -D__PPC__
endif
ifeq ($(CROSS_COMPILE),powerpc-openbsd-)
PLATFORM_CPPFLAGS+= -D__PPC__
endif
endif

ifeq ($(ARCH),arm)
ifeq ($(CROSS_COMPILE),powerpc-netbsd-)
PLATFORM_CPPFLAGS+= -D__ARM__
endif
ifeq ($(CROSS_COMPILE),powerpc-openbsd-)
PLATFORM_CPPFLAGS+= -D__ARM__
endif
endif

ifdef ARCH
sinclude $(TOPDIR)/$(ARCH)_config.mk	# include architecture dependend rules
endif

#########################################################################

CONFIG_SHELL := $(shell if [ -x "$$BASH" ]; then echo $$BASH; \
	else if [ -x /bin/bash ]; then echo /bin/bash; \
	else echo sh; fi ; fi)

ifeq ($(HOSTOS)-$(HOSTARCH),darwin-ppc)
HOSTCC	= cc
else
HOSTCC	= gcc
endif

HOSTCFLAGS	= -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer
HOSTSTRIP	= strip

#########################################################################

#
# Include the make variables (CC, etc...)
#
AS		= $(CROSS_COMPILE)as
LD		= $(CROSS_COMPILE)ld
CC		= $(CROSS_COMPILE)gcc
CXX		= $(CROSS_COMPILE)g++
CPP		= $(CC) -E
AR		= $(CROSS_COMPILE)ar
NM		= $(CROSS_COMPILE)nm
STRIP	= $(CROSS_COMPILE)strip
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
RANLIB	= $(CROSS_COMPILE)RANLIB

DBGFLAGS := -g #-DDEBUG

gccinc := $(shell $(CC) --print-file-name=include)

CPPFLAGS := $(DBGFLAGS) -I. -I$(gccinc) -I$(TOPDIR)/include -I$(TOPDIR)/include/AiSound5 -I$(TOPDIR)/include/neux

ifeq ($(ARCH),mips)
CPPFLAGS += -mips32 -mhard-float
endif

CFLAGS   := $(CPPFLAGS) -Wall -O2

ifeq ($(ARCH),mips)
LDFLAGS	+= -Bstatic $(PLATFORM_LDFLAGS) -EL
else
LDFLAGS	+= -Bstatic $(PLATFORM_LDFLAGS)
endif

#########################################################################

export	CONFIG_SHELL HOSTCC HOSTCFLAGS CROSS_COMPILE \
	AS LD CC CXX CPP AR NM STRIP OBJCOPY OBJDUMP \
	MAKE

export	PLATFORM_CPPFLAGS PLATFORM_RELFLAGS CPPFLAGS CFLAGS

#########################################################################

%.o: %.S
	$(CC) $(AFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CFLAGS) -c -o $@ $<

%.o: %.cxx
	$(CXX) $(CFLAGS) -c -o $@ $<

%.o: %.c++
	$(CXX) $(CFLAGS) -c -o $@ $<

#########################################################################
