del wdl.zip
cd..
zip -X9r wdl\wdl.zip wdl -i *.c *.cpp *.cc *.h *.m *.mm *.png *.ico *.txt *.bat
zip -X9r wdl\wdl.zip wdl -i *.rc *.plist *.dsp *.dsw *.pbxproj *.strings *.nib *.php *.exp *.vcproj *.vcxproj

zip -X9r wdl\wdl.zip wdl\iplug\readme.txt wdl\iplug\example\readme.txt wdl\iplug\example\img.zip
zip -X9r wdl\wdl.zip wdl\jnetlib\makefile 
zip -X9r wdl\wdl.zip wdl\jpeglib\readme 
zip -X9r wdl\wdl.zip wdl\giflib\authors wdl\giflib\changelog wdl\giflib\copying wdl\giflib\developers wdl\giflib\readme

cd wdl
