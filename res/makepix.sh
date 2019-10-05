#!/bin/sh

mkdir -p tmp

cat 16.png | png2ff >> tmp/16.ff
cat 32.png | png2ff >> tmp/32.ff
cat 64.png | png2ff >> tmp/64.ff

dd skip=16 count=256 if=tmp/16.ff of=tmp/16.bin bs=1
dd skip=16 count=256 if=tmp/32.ff of=tmp/32.bin bs=1
dd skip=16 count=256 if=tmp/64.ff of=tmp/64.bin bs=1

echo -n -e "\x00\x00\x00\x10\x00\x00\x00\x10" >> iconpix.bin
cat tmp/16.bin >> iconpix.bin
echo -n -e "\x00\x00\x00\x20\x00\x00\x00\x20" >> iconpix.bin
cat tmp/32.bin >> iconpix.bin
echo -n -e "\x00\x00\x00\x40\x00\x00\x00\x40" >> iconpix.bin
cat tmp/64.bin >> iconpix.bin

rm -rf ./tmp
