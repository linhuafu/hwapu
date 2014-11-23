
include $(TOPDIR)/config.mk

SHARED = 1

ifeq ($(SHARED),1)
CFLAGS += -fpic
ELF1 = $(TOPDIR)/libs/libvprompt.so
else
ELF2 = $(TOPDIR)/libs/libvprompt.a
endif
ELF2 = $(TOPDIR)/bin/vpromptd

CFLAGS += `$(PKG_CONFIG) gstreamer-0.10 --cflags` \
		`$(PKG_CONFIG) gstreamer-app-0.10 --cflags`
LIBS += `$(PKG_CONFIG) gstreamer-0.10 --libs` \
		`$(PKG_CONFIG) gstreamer-app-0.10 --libs` \
		`$(PKG_CONFIG) alsa --libs` \
		`$(PKG_CONFIG) libdaemon --libs` \
		-L$(TOPDIR)/libs -lAiSound5 -lAiSoundMtts -lplextalk-core \
		`${PKG_CONFIG} --libs cld` \
		-lrt

ASRCS	= $(wildcard *.s)
CSRCS	= $(wildcard *.c)
CPPSRCS	= $(wildcard *.cpp)
SRCS 	= $(ASRCS) $(CSRCS) $(CPPSRCS)

AOBJS	= $(patsubst %.s, %.o, $(ASRCS))
COBJS	= $(patsubst %.c, %.o, $(CSRCS))
CPPOBJS = $(patsubst %.cpp, %.o, $(CPPSRCS))
OBJS	= $(AOBJS) $(COBJS) $(CPPOBJS)

all: .depend $(ELF1) $(ELF2)

$(ELF1): libvprompt.o
ifeq ($(SHARED),1)
	$(CC) $(CFLAGS) -fpic -shared -o $(ELF1) libvprompt.o
else
	$(AR) -r $(ELF1) libvprompt.o
endif

$(ELF2): vpromptd.o
	$(CC) $(LDFLAGS) -o $(ELF2) vpromptd.o $(LIBS)

#########################################################################

.depend:	Makefile $(AOBJS:.o=.S) $(COBJS:.o=.c) $(CPPOBJS:.o=.cpp)
		$(CC) -M $(CFLAGS) $(AOBJS:.o=.S) $(COBJS:.o=.c) $(CPPOBJS:.o=.cpp) > $@

sinclude .depend

#########################################################################

clean:
	rm -f $(OBJS) .depend
	rm -f $(ELF1) $(ELF2)
