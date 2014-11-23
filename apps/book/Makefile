
include $(TOPDIR)/config.mk

ELF = $(TOPDIR)/bin/book

CFLAGS += -I$(TOPDIR)/include/neux  -I$(TOPDIR)/include/cld -I$(TOPDIR)/neux/nx -I${SYSROOT}/usr/include/microwin `${PKG_CONFIG} --cflags gstreamer-0.10` `${PKG_CONFIG} --cflags gstreamer-app-0.10` `${PKG_CONFIG} --cflags cld`
LIBS += -L$(TOPDIR)/libs -ldoc2text -lminizip -linfo -lplextalk-neux -lm -lwv2 -lplextalk-core -lvprompt -lnano-X -lasound -lAiSound5 -lAiSoundMtts -ldaisy -lunistring -lunibreak `${PKG_CONFIG} --libs gstreamer-0.10` `${PKG_CONFIG} --libs gstreamer-app-0.10` `${PKG_CONFIG} --libs cld` 

CFLAGS += -I./common -I./daisy -I./newtext

ASRCS	= $(wildcard *.s)
CSRCS	= $(wildcard ./common/*.c) $(wildcard ./daisy/*.c) $(wildcard *.c) $(wildcard ./newtext/*.c)
CPPSRCS	= $(wildcard ./common/*.cpp)
SRCS 	= $(ASRCS) $(CSRCS) $(CPPSRCS)

AOBJS	= $(patsubst %.s, %.o, $(ASRCS))
COBJS	= $(patsubst %.c, %.o, $(CSRCS))
CPPOBJS = $(patsubst %.cpp, %.o, $(CPPSRCS))
OBJS	= $(AOBJS) $(COBJS) $(CPPOBJS)

all: .depend $(OBJS)
	$(CC) $(LDFLAGS) -o $(ELF) $(OBJS) $(LIBS)
		xgettext -k_ -k__ *.c *.h ./common/*.c ./daisy/*.c ./newtext/*.c $(TOPDIR)/core/*.c $(TOPDIR)/neux/*.c $(TOPDIR)/neux/nx/*.c $(TOPDIR)/neux/wm/*.c $(TOPDIR)/apps/libinfo/*.c -o $(TOPDIR)/po/book.g.po


#########################################################################

.depend:	Makefile $(AOBJS:.o=.S) $(COBJS:.o=.c) $(CPPOBJS:.o=.cpp)
		$(CC) -M $(CFLAGS) $(AOBJS:.o=.S) $(COBJS:.o=.c) $(CPPOBJS:.o=.cpp) > $@

sinclude .depend

#########################################################################

clean:
	rm -f $(OBJS) .depend
	rm -f $(ELF)
