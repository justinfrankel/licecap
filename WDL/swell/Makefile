# use make 
# or make NOGDK=1
# or make DEBUG=1 
# etc

ARCH := $(shell uname -m)
UNAME_S := $(shell uname -s)


CFLAGS = -pipe -fvisibility=hidden -fno-math-errno -fPIC -DPIC -Wall -Wno-narrowing -Wno-sign-compare -Wno-unused-function -Wno-multichar -Wno-unused-result

ifeq ($(UNAME_S),Darwin)
  CFLAGS += -Wno-unused-private-field -DSWELL_FORCE_GENERIC
  ARCHSTR := $(ARCH)-darwin
  DLL_EXT=.dylib
else
  ARCHSTR := $(ARCH)-$(shell lsb_release -i -s | tr '[:upper:]' '[:lower:]')-$(shell lsb_release -c -s | tr '[:upper:]' '[:lower:]')
  DLL_EXT=.so
endif

ifndef ALLOW_WARNINGS
  CFLAGS += -Werror
endif
ifndef DEPRECATED_WARNINGS
  CFLAGS +=  -Wno-deprecated-declarations
endif

ifneq ($(filter arm%,$(ARCH)),)
  CFLAGS += -fsigned-char 
endif


ifdef DEBUG
CFLAGS += -O0 -g 
else
CFLAGS += -O2 
  ifneq ($(UNAME_S),Darwin)
    CFLAGS += -s
  endif
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

default: libSwell$(DLL_EXT)

.PHONY: clean 

SWELL_HEADERS=swell-dlggen.h swell-gdi-internalpool.h swell-menugen.h swell-types.h swell-functions.h swell-internal.h

swell.o: $(SWELL_HEADERS)
swell-ini.o: $(SWELL_HEADERS)
swell-wnd-generic.o: $(SWELL_HEADERS)
swell-dlg-generic.o: $(SWELL_HEADERS)
swell-menu-generic.o: $(SWELL_HEADERS)
swell-misc-generic.o: $(SWELL_HEADERS)
swell-gdi-generic.o: $(SWELL_HEADERS)
swell-gdi-lice.o: $(SWELL_HEADERS)
swell-kb-generic.o: $(SWELL_HEADERS)
swell-miscdlg-generic.o: $(SWELL_HEADERS)
swell-modstub-generic.o: $(SWELL_HEADERS)
swell-appstub-generic.o: $(SWELL_HEADERS)

libSwell$(DLL_EXT): $(OBJS)
	$(CXX) -shared -o $@ $(CFLAGS) $(LFLAGS) $^ $(LINKEXTRA)

test: $(OBJS) test.o
	$(CXX) -o test $(CFLAGS) $(LFLAGS) $^ $(LINKEXTRA)

libSwell.colortheme: swell-gdi-generic.cpp $(SWELL_HEADERS)
	$(CXX) $(CFLAGS) -o make-theme -DSWELL__MAKE_THEME swell-gdi-generic.cpp -lpthread
	./make-theme > libSwell.colortheme

clean: 
	-rm $(OBJS) libSwell$(DLL_EXT) make-theme libSwell.colortheme

dist-xz:
	-make NOGDK=1 clean
	make NOGDK=1
	tar cvJf libSwell-$(ARCHSTR)-headless.tar.xz libSwell$(DLL_EXT)
	-make clean
	make
	tar cvJf libSwell-$(ARCHSTR)-gtk3.tar.xz libSwell$(DLL_EXT)
