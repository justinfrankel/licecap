CC=gcc
CFLAGS=-O -g -DWDL_FFT_REALSIZE=8
LFLAGS=
CXX=g++

ifdef force32
CFLAGS += -arch i386
endif

OBJS=nseel-caltab.o nseel-compiler.o nseel-eval.o nseel-lextab.o nseel-ram.o nseel-yylex.o nseel-cfunc.o  ../fft.o

OBJS2=

UNAME_S := $(shell uname -s)

default: loose_eel

nseel-compiler.o: glue*.h ns-eel*.h
nseel-cfunc.o:  asm*.c ns-eel*.h
loose_eel.o: eel*.h ns-eel*.h
nseel-*.o: ns-eel*.h

ifdef MAXLOOP
  CFLAGS += -DNSEEL_LOOPFUNC_SUPPORT_MAXLEN=$(MAXLOOP)
else
  CFLAGS += -DNSEEL_LOOPFUNC_SUPPORT_MAXLEN=0
endif

ifeq ($(UNAME_S),Darwin)
  ifndef NO_GFX
    OBJS += ../lice/lice.o ../lice/lice_image.o ../lice/lice_line.o ../lice/lice_ico.o ../lice/lice_bmp.o ../lice/lice_textnew.o ../lice/lice_text.o ../lice/lice_arc.o
    CFLAGS += -DEEL_LICE_WANT_STANDALONE 

    CFLAGS += -mmacosx-version-min=10.5
    OBJS += ../swell/swell-wnd.o ../swell/swell-gdi.o ../swell/swell.o ../swell/swell-misc.o ../swell/swell-dlg.o ../swell/swell-menu.o ../swell/swell-kb.o
    LFLAGS += -lobjc -framework Cocoa -framework Carbon
  endif
endif



%.o : %.mm
	$(CXX) $(CXXFLAGS) -c -o $@ $^


ifdef PORTABLE
  CFLAGS +=  -DEEL_TARGET_PORTABLE
else
  ifeq ($(UNAME_S),Darwin)
    ifndef force32
      OBJS2 += asm-nseel-x64-macho.o
    endif
  endif

endif
CXXFLAGS=$(CFLAGS)

loose_eel: loose_eel.o $(OBJS) $(OBJS2)
	g++ -o $@ $^ $(CXXFLAGS) $(LFLAGS)

clean:
	-rm loose_eel.o $(OBJS)
