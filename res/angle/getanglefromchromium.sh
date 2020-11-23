#!/bin/sh

curl \
-L "https://download-chromium.appspot.com/dl/Mac?type=snapshots" \
-o ./chromium.zip

mkdir -p ./libs
unzip ./chromium.zip

cp \
"chrome-mac/Chromium.app/Contents/Frameworks/"\
"Chromium Framework.framework/Libraries/libGLESv2.dylib" \
"chrome-mac/Chromium.app/Contents/Frameworks/"\
"Chromium Framework.framework/Libraries/libEGL.dylib" \
./libs/
