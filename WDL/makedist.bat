del wdl.zip
cd..
zip -X9r wdl\wdl.zip wdl -i *.c *.cpp *.cc *.h *.m *.mm *.png *.ico *.txt *.bat *.rc *.plist *.dsp *.dsw *.pbxproj *.strings *.nib *.php *.exp *.vcproj *.vcxproj *_prefix.pch *.r *.xib *.sln *.icns

zip -X9r wdl\wdl.zip wdl\eel2\asm-nseel-x64-macho.o

zip -X9r wdl\wdl.zip wdl\iplug\example\img.zip

zip -X9r wdl\wdl.zip wdl\jnetlib\makefile 
zip -X9r wdl\wdl.zip wdl\jpeglib\readme 
zip -X9r wdl\wdl.zip wdl\giflib\authors wdl\giflib\changelog wdl\giflib\copying wdl\giflib\developers wdl\giflib\readme

cd wdl
