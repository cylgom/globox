# Globox
Globox is an experimental windowing library written as part of a project during
my third year at the [IMAC](https://www.ingenieur-imac.fr) school of engineering.

My goals were the following:
 - understand how display systems are designed under Linux, Windows and OS X
 - identify the challenges of writing a simple, low-level yet portable library

Globox was designed accordingly:
 - it is limited to windowing
 - it provides a simple, minimalistic API
 - it strives to be as low-level as possible
 - its only dependencies are the display systems libraries
 - window manipulation is limited to what is possible on every target platform
 - backend contexts are exposed for flexible event-loop integration

Globox works under Linux, Windows, OS X and is 100% C99 code, even the OS X part.
The provided makefile allows for easy cross-compilation from Linux.
*Compiling from any other OS is not supported*.

## Dependencies
### make-dependencies
Common
 - GNU Make

Linux
 - a C99 compiler
 - objcopy (icon embedding)

Linux + Wayland
 - wayland-scanner (wayland "standard" protocol extension)

Windows
 - mingw-w64 (cross-compilation tools, specific components listed below)
 - mingw-w64-crt-bin
 - mingw-w64-binutils-bin
 - mingw-w64-winpthreads-bin
 - mingw-w64-headers-bin
 - mingw-w64-gcc-bin
 - wine64 (optional, for `make run`)

OS X
 - osxcross (cross-compilation tools)
 - objconv (icon embedding)

### library dependencies
#### Software-rendering context
Linux
 - POSIX.1-2008 functions (low-level operations)

Linux + X11
 - xcb (display server API)
 - xcb-shm (shared memory mechanism)
 - xcb-randr (screen info functions)

Linux + Wayland
 - libwayland-client (display server API)
 - xdg-shell ("standard" protocol extension)

Windows
 - win32 (display server API)
 - gdi32 (direct memory access mechanism)

OS X
 - Objective-C runtime (Objective-C functionnalities)
 - Objective-C message (Objective-C communication)
 - AppKit (display server API)

#### Hardware-rendering contexts
At the moment, only Linux + X11 is supported.

Linux + X11 + Vulkan
 - libvulkan (Vulkan driver)
 - libVkLayer_khronos_validation (Vulkan validation layers)

Linux + X11 + OpenGL
 - libX11 (OpenGL driver)
 - libX11-xcb (OpenGL context integration in xcb)
 - libGL (OpenGL API)

## Building
Globox is not dynamic (yet) and you must choose your target context type and
display system before compiling. This is done through environment variables:

Set `RENDER` to
 - `swr`, for software rendering
 - `ogl`, for OpenGL (at the moment, only X11 is supported)
 - `vlk`, for Vulkan (at the moment, only X11 is supported)

Set `BACKEND` to
 - `x11`, for *Linux + X11* windows
 - `wayland`, for *Linux + Wayland* windows
 - `win`, for *Windows + Win32/GDI* windows
 - `quartz`, for *Mac OS X + Quartz* windows

You can also change the default values (`swr`/`x11`) directly in the `makefile`.
To compile, simply run make:
```
RENDER=swr BACKEND=wayland make
```

## Running
For Linux and Windows (if you have wine64 installed) you can just
```
make run
```

For OS X,
[macOS-Simple-KVM](https://github.com/foxlet/macOS-Simple-KVM)
is a very good solution until
[darling](https://github.com/darlinghq/darling.git)
works properly.

## Greetings
My teacher [Mr. Biri](https://github.com/venceslas) for approving this project.
Andrew Hulshult, whose music I massively listened to while coding this heresy
