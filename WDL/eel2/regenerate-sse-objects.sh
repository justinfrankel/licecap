# regenerates SSE objects
rm -f asm-nseel-x64-macho.o asm-nseel-x64.obj foo.o
nasm -f win64 asm-nseel-x64-sse.asm -o asm-nseel-x64.obj || echo "error assembling windows SSE object"
nasm -D AMD64ABI -f macho64 --prefix _ asm-nseel-x64-sse.asm -o asm-nseel-x64-macho.o  || echo "error assembling macOS-sse object"
echo > foo.c
clang -arch arm64 -c -o foo.o foo.c || echo "error compiling arm64 stub"
lipo foo.o asm-nseel-x64-macho.o -create -output asm-nseel-multi-macho.o || echo "error making asm-nseel-multi-macho.o"
rm -f -- foo.c foo.o
