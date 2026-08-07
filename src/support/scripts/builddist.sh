#!/bin/csh -f

echo Distributing version $argv
echo Run this script from support/scripts!

cd ../..

echo Now in
pwd

mkdir jacob"$argv"

cd jacob"$argv"

echo Now in
pwd


echo Copying source files...
mkdir src
cp ../*.cpp src
cp ../*.h src
cp ../*.txt src
cp ../*.TXT src
cp ../*.c src
cp ../*.cfg src

mkdir src/support
mkdir src/support/enummaker
cp ../support/enummaker/Makefile src/support/enummaker
cp ../support/enummaker/enummaker.cpp src/support/enummaker
cp ../support/enummaker/enummaker.sln src/support/enummaker
cp ../support/enummaker/enummaker.vcproj src/support/enummaker
mkdir src/support/scripts
cp ../support/scripts/builddist.sh src/support/scripts

mkdir src/lib
mkdir src/rooms
cp ../rooms/*.map src/rooms
mkdir src/windows
cp ../windows/*.sln src/windows
cp ../windows/*.vcproj src/windows

mkdir src/linux
cp ../linux/Makefile src/linux
cp ../linux/*.TXT src/linux
cp ../linux/*.sh src/linux

echo Copying Windows Files

cp ../text.txt .
cp ../README.TXT .
mkdir rooms
cp ../rooms/*.map rooms
mkdir windows
cp ../windows/Release/*.exe windows
cp ../windows/*.dll windows
cp ../windows/*.lib windows
cp ../windows/*.png windows

cp "c:/Program Files (x86)/Microsoft Visual Studio 8/VC/redist/x86/Microsoft.VC80.CRT"/* windows

cp ../jacob.cfg .

mkdir music
cp ../music/*.ogg music

echo All done!  Zipping.

cd ..
rm -f jacob"$argv".zip
7z a jacob"$argv".zip jacob"$argv"

