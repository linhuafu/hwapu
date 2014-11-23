
include $(TOPDIR)/config.mk

SHARED = 1

ifeq ($(SHARED),1)
CFLAGS += -fpic
ELF = $(TOPDIR)/libs/libinfo.so
else
ELF = $(TOPDIR)/libs/libinfo.a
endif

CFLAGS += `${PKG_CONFIG} --cflags libxml-2.0` -I$(TOPDIR)/include/neux -I$(TOPDIR)/neux/nx -I${SYSROOT}/usr/include/microwin

CFLAGS += -I/opt/clfs-mipsel-linux3.9.11-hard_float/usr/include/glib-2.0
CFLAGS += -I/opt/clfs-mipsel-linux3.9.11-hard_float/usr/lib/glib-2.0/include

LIBS += -lnano-X
LIBS += `${PKG_CONFIG} --libs libxml-2.0`

ASRCS	= $(wildcard *.s)
CSRCS	= $(wildcard *.c) $(wildcard nx/*.c) $(wildcard wm/*.c)
CPPSRCS	= $(wildcard *.cpp)
SRCS 	= $(ASRCS) $(CSRCS) $(CPPSRCS)

AOBJS	= $(patsubst %.s, %.o, $(ASRCS))
COBJS	= $(patsubst %.c, %.o, $(CSRCS))
CPPOBJS = $(patsubst %.cpp, %.o, $(CPPSRCS))
OBJS	= $(AOBJS) $(COBJS) $(CPPOBJS)

all: .depend $(OBJS)
ifeq ($(SHARED),1)
	$(CC) $(CFLAGS) -fpic -shared -o $(ELF) $(OBJS) $(LIBS)
else
	$(AR) -r $(ELF) $(OBJS)
endif
	xgettext -k_ *.c -o $(TOPDIR)/po/libinfo.g.po
#########################################################################

.depend:	Makefile $(AOBJS:.o=.S) $(COBJS:.o=.c) $(CPPOBJS:.o=.cpp)
		$(CC) -M $(CFLAGS) $(AOBJS:.o=.S) $(COBJS:.o=.c) $(CPPOBJS:.o=.cpp) > $@

sinclude .depend

#########################################################################

clean:
	rm -f $(OBJS) .depend
	rm -f $(ELF)
