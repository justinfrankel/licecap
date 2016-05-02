# use make -f Makefile.libSwell 
# or make -f Makefile.libSwell NOGDK=1
# or make -f Makefile.libSwell DEBUG=1 
# etc

ARCH := $(shell uname -m)
ARCHSTR := $(ARCH)-$(shell lsb_release -i -s | tr '[:upper:]' '[:lower:]')-$(shell lsb_release -c -s | tr '[:upper:]' '[:lower:]')

CFLAGS = -pipe -fvisibility=hidden -fno-strict-aliasing -fno-math-errno -fPIC -DPIC -Wall -Wno-narrowing -Wno-sign-compare -Wno-unused-function

ifneq ($(filter arm%,$(ARCH)),)
  CFLAGS += -fsigned-char 
else
  CFLAGS += -malign-double 
endif

ifdef DEBUG
CFLAGS += -O0 -g 
else
CFLAGS += -O2 -s
endif

LINKEXTRA =  -lpthread -ldl 


EXTRA_OBJS = 




vpath %.cpp .. ../lice 

SWELL_OBJS = swell.o swell-ini.o swell-miscdlg-generic.o swell-wnd-generic.o swell-menu-generic.o swell-kb-generic.o swell-dlg-generic.o \
             swell-gdi-generic.o swell-misc-generic.o swell-appstub-generic.o swell-modstub-generic.o swell-gdi-lice.o

LICE_OBJS = lice.o  lice_arc.o lice_colorspace.o lice_line.o lice_text.o \
            lice_textnew.o lice_ico.o lice_bmp.o

OBJS = $(SWELL_OBJS)

ifndef NOGDK
  ifdef GDK2
    CFLAGS += -DSWELL_TARGET_GDK=2 $(shell pkg-config --cflags gdk-2.0)
    LINKEXTRA += $(shell pkg-config --libs gdk-2.0)
  else
    CFLAGS += -DSWELL_TARGET_GDK=3 $(shell pkg-config --cflags gdk-3.0)
    LINKEXTRA += $(shell pkg-config --libs gdk-3.0)
  endif
  CFLAGS += -DSWELL_LICE_GDI
  OBJS += $(LICE_OBJS)

  ifndef NOFREETYPE
    CFLAGS += -DSWELL_FREETYPE $(shell freetype-config --cflags)
    LINKEXTRA += $(shell freetype-config --libs)
  endif
endif

CXXFLAGS = $(CFLAGS)

default: libSwell.so

.PHONY: clean 

libSwell.so: $(OBJS)
	$(CXX) -shared -o $@ $(CFLAGS) $(LFLAGS) $(LINKEXTRA) $^ 

test: $(OBJS) test.o
	$(CXX) -o test $(CFLAGS) $(LFLAGS) $(LINKEXTRA) $^

clean: 
	-rm $(OBJS) libSwell.so

dist-xz:
	-make NOGDK=1 clean
	make NOGDK=1
	tar cvJf libSwell-$(ARCHSTR)-headless.tar.xz libSwell.so
	-make clean
	make
	tar cvJf libSwell-$(ARCHSTR)-gtk3.tar.xz libSwell.so
