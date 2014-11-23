
include $(TOPDIR)/config.mk

SHARED = 1

ifeq ($(SHARED),1)
CFLAGS += -fpic
ELF = $(TOPDIR)/libs/libplextalk-neux.so
else
ELF = $(TOPDIR)/libs/libplextalk-neux.a
endif

CFLAGS += -I$(TOPDIR)/include/neux -I./nx -I./wm -I${SYSROOT}/usr/include/microwin
LIBS +=-L$(TOPDIR)/libs -lnano-X -lunibreak -linfo

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

#########################################################################

.depend:	Makefile $(AOBJS:.o=.S) $(COBJS:.o=.c) $(CPPOBJS:.o=.cpp)
		$(CC) -M $(CFLAGS) $(AOBJS:.o=.S) $(COBJS:.o=.c) $(CPPOBJS:.o=.cpp) > $@

sinclude .depend

#########################################################################

clean:
	rm -f $(OBJS) .depend
	rm -f $(ELF)
