nasm -f win64 asm-nseel-x64-sse.asm -o asm-nseel-x64.obj

nasm -D AMD64ABI -f macho64 --prefix _ asm-nseel-x64-sse.asm  -o asm-nseel-x64-macho.o && echo > foo.c && clang -arch arm64 -c -o foo.o foo.c && lipo foo.o asm-nseel-x64-macho.o -create -output asm-nseel-multi-macho.o ; rm foo.c
