
include $(TOPDIR)/config.mk

CFLAGS += -fpic -O2 -g
ELF = $(TOPDIR)/libs/libdoc2text.so

CFLAGS += `$(PKG_CONFIG) glib-2.0 --cflags` \
           `$(PKG_CONFIG) libxml-2.0 --cflags` \
           `$(PKG_CONFIG) libgsf-1 --cflags` \
           `$(PKG_CONFIG) minizip --cflags` \
           `$(PKG_CONFIG) zlib --cflags` \
           -I $(SYSROOT)/usr/include/wv2 \
           -fpermissive

LIBS =    `$(PKG_CONFIG) libxml-2.0 --libs` \
          `$(PKG_CONFIG) libgsf-1 --libs` \
          `$(PKG_CONFIG) glib-2.0 --libs` \
          `$(PKG_CONFIG) minizip --libs` \
          `$(PKG_CONFIG) zlib --libs` \
          -lwv2

ASRCS	= $(wildcard *.s)
CSRCS	= $(wildcard *.c)
CPPSRCS	= $(wildcard *.cpp)
SRCS 	= $(ASRCS) $(CSRCS) $(CPPSRCS)

AOBJS	= $(patsubst %.s, %.o, $(ASRCS))
COBJS	= $(patsubst %.c, %.o, $(CSRCS))
CPPOBJS = $(patsubst %.cpp, %.o, $(CPPSRCS))
OBJS	= $(AOBJS) $(COBJS) $(CPPOBJS)

all: .depend $(ELF)

$(ELF): $(OBJS)
	$(CXX) $(CFLAGS) -fpic -shared -o $@ $^

#########################################################################

.depend:	Makefile $(AOBJS:.o=.S) $(COBJS:.o=.c) $(CPPOBJS:.o=.cpp)
		$(CC) -M $(CFLAGS) $(AOBJS:.o=.S) $(COBJS:.o=.c) $(CPPOBJS:.o=.cpp) > $@

sinclude .depend

#########################################################################

clean:
	rm -f $(OBJS) .depend
	rm -f $(ELF1) $(ELF2)
