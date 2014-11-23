
include $(TOPDIR)/config.mk

ELF = $(TOPDIR)/bin/book

CFLAGS += -I$(TOPDIR)/include/neux -I${SYSROOT}/usr/include/microwin `${PKG_CONFIG} --cflags gstreamer-0.10` \
`${PKG_CONFIG} --cflags gstreamer-app-0.10`
LIBS += -L$(TOPDIR)/libs -lplextalk-neux -lplextalk-core -lvprompt -lnano-X -lasound `${PKG_CONFIG} --libs gstreamer-0.10` \
`${PKG_CONFIG} --libs gstreamer-app-0.10` -ldoc2text -lunibreak -lunistring -lAiSound5

ASRCS	= $(wildcard *.s)
CSRCS	= $(wildcard *.c)
CPPSRCS	= $(wildcard *.cpp)
SRCS 	= $(ASRCS) $(CSRCS) $(CPPSRCS)

AOBJS	= $(patsubst %.s, %.o, $(ASRCS))
COBJS	= $(patsubst %.c, %.o, $(CSRCS))
CPPOBJS = $(patsubst %.cpp, %.o, $(CPPSRCS))
OBJS	= $(AOBJS) $(COBJS) $(CPPOBJS)

all: .depend $(OBJS)
	$(CC) $(LDFLAGS) -o $(ELF) $(OBJS) $(LIBS)

#########################################################################

.depend:	Makefile $(AOBJS:.o=.S) $(COBJS:.o=.c) $(CPPOBJS:.o=.cpp)
		$(CC) -M $(CFLAGS) $(AOBJS:.o=.S) $(COBJS:.o=.c) $(CPPOBJS:.o=.cpp) > $@

sinclude .depend

#########################################################################

clean:
	rm -f $(OBJS) .depend
	rm -f $(ELF)
