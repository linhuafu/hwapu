
include $(TOPDIR)/config.mk

SHARED = 1

ifeq ($(SHARED),1)
CFLAGS += -fpic
ELF = $(TOPDIR)/libs/libplextalk-core.so
else
ELF = $(TOPDIR)/libs/libplextalk-core.a
endif

CFLAGS += `${PKG_CONFIG} --cflags libxml-2.0` \
			`${PKG_CONFIG} --cflags cld` \
			-I${SYSROOT}/usr/include/microwin
LIBS += `${PKG_CONFIG} --libs libxml-2.0` \
		`${PKG_CONFIG} --libs cld` -lasound

ASRCS	= $(wildcard *.s)
CSRCS	= $(wildcard *.c)
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

#########################################################################

.depend:	Makefile $(AOBJS:.o=.S) $(COBJS:.o=.c) $(CPPOBJS:.o=.cpp)
		$(CC) -M $(CFLAGS) $(AOBJS:.o=.S) $(COBJS:.o=.c) $(CPPOBJS:.o=.cpp) > $@

sinclude .depend

#########################################################################

clean:
	rm -f $(OBJS) .depend
	rm -f $(ELF)
