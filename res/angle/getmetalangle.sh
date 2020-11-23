#!/bin/sh

curl \
-L "https://github.com/kakashidinho/metalangle/releases/download/gles3-0.0.4/MetalANGLE.dylib.mac.zip" \
-o ./metalangle.zip

mkdir -p ./libs
cd ./libs

unzip ../metalangle.zip
mv ./include ..
