#!/bin/sh

# cloning repos
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
git clone https://chromium.googlesource.com/angle/angle

# xcode trick
sudo xcode-select -s /Applications/Xcode.app/Contents/Developer

# create dirs
mkdir bin
mkdir libs

# python2 trick
ln -s /usr/bin/python2 ./bin/python
ln -s /usr/bin/python2-config ./bin/python-config

# add depot_tools to environment and finish setting up python2 trick
workdir=$(pwd)
export PATH=$workdir/bin/:$workdir/depot_tools/:$PATH

# prepare repo
cd angle
python scripts/bootstrap.py
gclient sync
git checkout master

# see build/install-build-deps.sh for dependencies
gn gen out/Release

# user input
echo "you will have to write \"is_debug = false\""
sleep 10
gn args out/Release

# build
autoninja -C out/Release
cp out/Release/libGLESv2.dylib out/Release/libEGL.dylib ../libs/
