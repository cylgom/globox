# Globox

## Contexts backends details
X11
 - `software` provides CPU and GPU buffers using XCB's SHM module.
 - `egl` provides OpenGL and OpenGL ES contexts using XCB and EGL
   (which relies on GLX/libX11 in this case).
   Transparency is not supported because of a bug in EGL.
 - `glx` provides OpenGL contexts using XCB and GLX/libX11.
 - `vk` provides Vulkan contexts using Vulkan's WSI
   It is not yet implemented.

Wayland
 - `software` provides an abstract buffer using libwayland-client
 - `egl` provides OpenGL and OpenGL ES contexts using only EGL
 - `vk` provides Vulkan contexts using Vulkan's WSI
   It is not yet implemented.

Windows
 - `software` provides an abstract buffer using GDI
 - `egl` provides OpenGL contexts using an internal WGL wrapper
   It is not yet implemented.
 - `wgl` provides OpenGL contexts using only WGL
 - `vk` provides Vulkan contexts using Vulkan's WSI
   It is not yet implemented.

Mac OS
 - `software` provides an abstract buffer using CoreImage
   (which relies on gallium3D software OpenGL rendering)
 - `egl` provides OpenGL ES contexts using ANGLE
   (which implements OpenGL ES on top of metal)
   It is not yet implemented.
 - `vk` provides Vulkan contexts using MoltenVK
   (which implements Vulkan on top of metal)
   It is not yet implemented.

## Compiling
### Wayland
Globox supports Plasma's background blur Wayland protocol, and although we will
obviously stay compatible with compositors which do not support this protocol,
it is required to install its .xml files in order to compile.

By making this mandatory we make sure the feature is available to Plasma users,
without having to build a specific binary for that purpose.

Under ArchLinux, Plasma's protocols can be installed using the 
`plasma-wayland-protocols` package found in the `Extra` repo.

The official protocols repository can be found on
[invent.kde.org](https://invent.kde.org/libraries/plasma-wayland-protocols).

### MacOS
#### Cross-compiling from Linux

#### Compiling from MacOS
The MacOS backend can also be compiled from MacOS for easier debugging.
*Do not download Xcode just for globox, it would be completely overkill!*
Instead, you can get the bare MacOS SDK from Apple's "Software Update" servers.
This will be way faster and doesn't even require registering an Apple account ;)
For this, simply use [instant_macos_sdk](https://github.com/nullgemm/instant_macos_sdk).

If you installed the SDK using the method recommended above, you must also
install git, as it usually comes bundled with Xcode.
You can use [Homebrew](https://brew.sh) for this.
```
brew install git
```

The example makefile also requires some extra tools, starting with `objcopy`
```
brew install binutils
```

After installation, the binary can be found under
```
/usr/local/Cellar/binutils/VERSION_NUMBER/bin/objcopy
```

Another tool required is `objconv`, which sadly can't be installed with `brew`
```
curl -O https://www.agner.org/optimize/objconv.zip
```

You will have to unzip it as well as the `sources.zip` archive it contains.
Run the `build.sh` script to generate an `objconv` binary.
