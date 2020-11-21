#!/bin/sh

echo "# making tmp folder"
mkdir ./tmp
cd ./tmp

echo "# downloading"
curl -O https://www.agner.org/optimize/objconv.zip

echo "# extracting"
unzip objconv.zip
unzip source.zip

echo "# building"
./build.sh

echo "# moving binary"
mv ./objconv ..

echo "# removing tmp folder"
cd ..
rm -rf ./tmp
