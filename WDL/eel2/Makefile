CC=gcc
CFLAGS=-O -g -I/Developer/SDKs/MacOSX10.6.sdk/usr/include/ -DWDL_FFT_REALSIZE=8
LFLAGS=-L/Developer/SDKs/MacOSX10.6.sdk/usr/lib
CXX=g++

OBJS=nseel-caltab.o nseel-compiler.o nseel-eval.o nseel-lextab.o nseel-ram.o nseel-yylex.o nseel-cfunc.o  ../fft.o

OBJS2=

UNAME_S := $(shell uname -s)

default: test

nseel-compiler.o: glue*.h
nseel-cfunc.o:  asm*.c

ifdef PORTABLE
  CFLAGS +=  -DEEL_TARGET_PORTABLE
else
  ifeq ($(UNAME_S),Darwin)
    OBJS2 += asm-nseel-x64-macho.o
  endif

endif
CXXFLAGS=$(CFLAGS)

test: test.o $(OBJS) $(OBJS2)
	g++ -o $@ $^ $(CXXFLAGS) $(LFLAGS)

clean:
	-rm test.o $(OBJS)
