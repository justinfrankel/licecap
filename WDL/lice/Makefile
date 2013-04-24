CFLAGS=-O2 -s -D_LICE_NO_SYSBITMAPS_
CC=gcc
CXX=g++
WDL_PATH=..

CXXFLAGS=$(CFLAGS)

vpath %.c $(WDL_PATH)/zlib $(WDL_PATH)/libpng $(WDL_PATH)/jpeglib  $(WDL_PATH)/giflib
vpath %.cpp test/

ZLIB_OBJS = compress.o adler32.o crc32.o deflate.o gzio.o infback.o inffast.o inflate.o inftrees.o trees.o uncompr.o zutil.o unzip.o ioapi.o zip.o unzip.o

PNGLIB_OBJS = png.o pngerror.o pngget.o pngmem.o pngpread.o pngread.o pngrio.o pngrtran.o pngrutil.o pngset.o pngtrans.o 

JPEGLIB_OBJS = jcomapi.o jdapimin.o jdapistd.o jdatadst.o jdatasrc.o jdcoefct.o jdcolor.o jddctmgr.o jdhuff.o jdinput.o jdmainct.o jdmarker.o \
               jdmaster.o jdmerge.o jdphuff.o jdpostct.o jdsample.o jerror.o jfdctflt.o jfdctfst.o jfdctint.o jidctflt.o jidctfst.o jidctint.o \
               jidctred.o jmemmgr.o jmemnobs.o jquant1.o jquant2.o jutils.o

GIFLIB_OBJS = dgif_lib.o egif_lib.o gifalloc.o gif_hash.o 

LICEOBJS = lice.o lice_gif.o lice_gif_write.o lice_image.o lice_jpg.o lice_png.o  lice_palette.o

imgs2gif: $(LICEOBJS) $(JPEGLIB_OBJS) $(PNGLIB_OBJS) $(ZLIB_OBJS) $(GIFLIB_OBJS) imgs2gif.o
	$(CXX) $(CFLAGS) -o $@ $^
