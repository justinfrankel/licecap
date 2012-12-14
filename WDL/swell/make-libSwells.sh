#!/bin/sh
DESCSTR=libSwell-`uname -m`-`lsb_release -i -s | tr '[:upper:]' '[:lower:]'`-`lsb_release -c -s | tr '[:upper:]' '[:lower:]'`
make -f Makefile.libSwell NOGDK=1 clean 
make -f Makefile.libSwell NOGDK=1 || exit 1
tar cvJf $DESCSTR-headless.tar.xz libSwell.so || exit 1

make -f Makefile.libSwell clean 
make -f Makefile.libSwell || exit 1
tar cvJf $DESCSTR-gtk3.tar.xz libSwell.so || exit 1

