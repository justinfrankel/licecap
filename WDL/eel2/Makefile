CC=gcc
CFLAGS=-O -g
CXX=g++

OBJS=nseel-caltab.o nseel-compiler.o nseel-eval.o nseel-lextab.o nseel-ram.o nseel-yylex.o

default: test

nseel-compiler.o: glue*.h
nseel-cfunc.o:  asm*.c

ifdef PORTABLE
  CFLAGS +=  -DEEL_TARGET_PORTABLE
  OBJS += nseel-cfunc.o 
endif
CXXFLAGS=$(CFLAGS)

test: test.o $(OBJS)
	g++ -o $@ $^ $(CXXFLAGS)

clean:
	-rm test.o $(OBJS)
