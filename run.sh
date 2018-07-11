#!/bin/sh
dir=`dirname $0`
cd $dir
FILE=$(basename "$1" .zip)
FILE_RM="./roms/$FILE.zip"
FILE_EX="./cache/$FILE.cache"

if [ -f $FILE_RM ]; then
	echo "ROM exists. Don't copy"
else
	echo "ROM does not exist, Do copy"
	cp $1 "./roms/$FILE.zip"
fi

if [ -f $FILE_EX ]; then
	echo "Cache exists. Run emulator"
else
	echo "Cache for Rom does not exist. Run Converter"
	./romconv.elf ecofghtr
fi
./cps2emu.elf ./ecofghtr.zip
