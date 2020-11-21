# program
## program name
NAME = globox
## program execution command
CMD = ./$(NAME)
## valgrind execution arguments
VALGRIND = --show-error-list=yes --show-leak-kinds=all --track-origins=yes --leak-check=full --suppressions=../res/valgrind.supp

# compiler
## compiler name
CC = gcc
## compiler options
FLAGS = -std=c99 -pedantic -g
## warning settings
FLAGS+= -Wall -Wextra -Werror=vla -Werror -Wno-unused-parameter -Wno-address-of-packed-member
## error settings
FLAGS+= -DGLOBOX_ERROR_LOG_BASIC
FLAGS+= -DGLOBOX_ERROR_LOG_THROW
FLAGS+= -DGLOBOX_ERROR_LOG_DEBUG
#FLAGS+= -DGLOBOX_ERROR_SKIP
#FLAGS+= -DGLOBOX_ERROR_ABORT

# source
## folders
BIND = bin
OBJD = obj
SRCD = src
INCD = inc
SUBD = sub
RESD = res
## header files
INCL = -I$(SRCD)
INCL+= -I$(INCD)
## main code files
SRCS = $(SRCD)/globox.c
SRCS+= $(SRCD)/globox_error.c
SRCS_OBJS =

# targets
PLATFORM ?= MACOS
CONTEXT ?= EGL

## X11
ifeq ($(PLATFORM), X11)
SRCS+= $(SRCD)/x11/globox_x11.c
SRCS_OBJS+= $(OBJD)/$(RESD)/icon/iconpix.o
LINK+= -lxcb
FLAGS+= -DGLOBOX_PLATFORM_X11

ifeq ($(CONTEXT), SOFTWARE)
FLAGS+= -DGLOBOX_CONTEXT_SOFTWARE
SRCS+= example/software.c
SRCS+= $(SRCD)/x11/software/globox_x11_software.c
LINK+= -lxcb-shm
LINK+= -lxcb-randr
LINK+= -lxcb-render
endif

ifeq ($(CONTEXT), EGL)
FLAGS+= -DGLOBOX_CONTEXT_EGL
SRCS+= example/egl.c
SRCS+= $(SRCD)/x11/egl/globox_x11_egl.c
LINK+= -lGL
LINK+= -lEGL
endif

ifeq ($(CONTEXT), GLX)
FLAGS+= -DGLOBOX_CONTEXT_GLX
SRCS+= example/glx.c
SRCS+= $(SRCD)/x11/glx/globox_x11_glx.c
LINK+= -lGL
LINK+= -lX11
LINK+= -lX11-xcb
LINK+= -lXrender
endif
endif

## WAYLAND
ifeq ($(PLATFORM), WAYLAND)
SRCS+= $(SRCD)/wayland/globox_wayland.c
SRCS+= $(SRCD)/wayland/globox_wayland_callbacks.c
SRCS+= $(INCD)/xdg-shell-protocol.c
SRCS+= $(INCD)/xdg-decoration-protocol.c
SRCS+= $(INCD)/kde-blur-protocol.c
SRCS+= $(INCD)/zwp-relative-pointer-protocol.c
SRCS+= $(INCD)/zwp-pointer-constraints-protocol.c
SRCS_OBJS+= $(OBJD)/$(RESD)/icon/iconpix.o
LINK+= -lwayland-client -lrt
FLAGS+= -DGLOBOX_PLATFORM_WAYLAND

ifeq ($(CONTEXT), SOFTWARE)
FLAGS+= -DGLOBOX_CONTEXT_SOFTWARE
SRCS+= example/software.c
SRCS+= $(SRCD)/wayland/software/globox_wayland_software.c
SRCS+= $(SRCD)/wayland/software/globox_wayland_software_helpers.c
endif

ifeq ($(CONTEXT), EGL)
FLAGS+= -DGLOBOX_CONTEXT_EGL
SRCS+= example/egl.c
SRCS+= $(SRCD)/wayland/egl/globox_wayland_egl.c
SRCS+= $(SRCD)/wayland/egl/globox_wayland_egl_helpers.c
LINK+= -lGL
LINK+= `pkg-config wayland-client wayland-egl --cflags --libs`
LINK+= `pkg-config egl glesv2 --cflags --libs`
endif
endif

## MACOS
ifeq ($(PLATFORM), MACOS)
CMD = ./$(NAME).app
SRCS+= $(SRCD)/macos/globox_macos.c
SRCS+= $(SRCD)/macos/globox_macos_helpers.c
SRCS_OBJS+= $(OBJD)/$(RESD)/icon/iconpix_mach.o
LINK+= -framework AppKit
FLAGS+= -DGLOBOX_PLATFORM_MACOS

ifeq ($(CONTEXT), SOFTWARE)
FLAGS+= -DGLOBOX_CONTEXT_SOFTWARE
SRCS+= example/software.c
SRCS+= $(SRCD)/macos/software/globox_macos_software.c
LINK+=
endif

ifeq ($(CONTEXT), EGL)
FLAGS+= -DGLOBOX_CONTEXT_EGL
SRCS+= example/egl.c
SRCS+= $(SRCD)/macos/egl/globox_macos_egl.c
LINK+= -L$(RESD)/angle/libs
LINK+= -lGLESv2
LINK+= -lEGL
endif
endif

# object files list
SRCS_OBJS+= $(patsubst %.c,$(OBJD)/%.o,$(SRCS))

# bin
.PHONY: final
ifeq ($(PLATFORM), MACOS)
final: $(BIND)/$(NAME).app
else
final: $(BIND)/$(NAME)
endif

## wayland protocols
ifeq ($(PLATFORM), WAYLAND)
$(INCD):
	@echo "generating wayland protocol extensions source files"
	@mkdir $@
	@wayland-scanner private-code \
	< /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml \
	> $@/xdg-shell-protocol.c
	@wayland-scanner client-header \
	< /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml \
	> $@/xdg-shell-client-protocol.h
	@wayland-scanner private-code \
	< /usr/share/wayland-protocols/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml \
	> $@/xdg-decoration-protocol.c
	@wayland-scanner client-header \
	< /usr/share/wayland-protocols/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml \
	> $@/xdg-decoration-client-protocol.h
	@wayland-scanner private-code \
	< /usr/share/plasma-wayland-protocols/blur.xml \
	> $@/kde-blur-protocol.c
	@wayland-scanner client-header \
	< /usr/share/plasma-wayland-protocols/blur.xml \
	> $@/kde-blur-client-protocol.h
	@wayland-scanner private-code \
	< /usr/share/wayland-protocols/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml \
	> $@/zwp-pointer-constraints-protocol.c
	@wayland-scanner client-header \
	< /usr/share/wayland-protocols/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml \
	> $@/zwp-pointer-constraints-protocol.h
	@wayland-scanner private-code \
	< /usr/share/wayland-protocols/unstable/relative-pointer/relative-pointer-unstable-v1.xml \
	> $@/zwp-relative-pointer-protocol.c
	@wayland-scanner client-header \
	< /usr/share/wayland-protocols/unstable/relative-pointer/relative-pointer-unstable-v1.xml \
	> $@/zwp-relative-pointer-protocol.h
endif

## icon
$(RESD)/icon/iconpix.bin:
	@echo "generating icons pixmap"
	@cd $(RESD)/icon && ./makepix.sh

$(OBJD)/$(RESD)/icon/iconpix.o: $(RESD)/icon/iconpix.bin
	@echo "building icon object"
	@mkdir -p $(@D)
	@objcopy -I binary -O elf64-x86-64 -B i386:x86-64 \
	--redefine-syms=$(RESD)/icon/syms.map \
	--rename-section .data=.iconpix \
	$< $@

## macOS icon
$(RESD)/objconv/objconv:
	@echo "making objconv"
	@cd ./$(RESD)/objconv && ./makeobjconv.sh

$(OBJD)/$(RESD)/icon/iconpix_mach.o: $(RESD)/icon/iconpix.bin $(RESD)/objconv/objconv
	@echo "building icon object object and converting to mach-o (bug workaround)"
	@mkdir -p $(@D)
	@/usr/local/Cellar/binutils/*/bin/objcopy -I binary -O elf64-x86-64 -B i386:x86-64 \
	--redefine-syms=$(RESD)/icon/syms.map \
	--rename-section .data=.iconpix \
	$< $(OBJD)/$(RESD)/icon/iconpix.o
	@./$(RESD)/objconv/objconv -fmac64 -nu+ -v0 $(OBJD)/$(RESD)/icon/iconpix.o $@

## macOS ANGLE
$(RESD)/angle/libs:
	@echo "making ANGLE"
	@cd ./$(RESD)/angle && ./makeangle.sh

## compilation
$(OBJD)/%.o: %.c
	@echo "building object $@"
	@mkdir -p $(@D)
	@$(CC) $(INCL) $(FLAGS) -c -o $@ $<

$(BIND)/$(NAME): $(SRCS_OBJS)
	@echo "compiling executable $@"
	@mkdir -p $(@D)
	@$(CC) -o $@ $^ $(LINK)

$(BIND)/$(NAME).app: $(RESD)/angle/libs $(BIND)/$(NAME)
	@echo "renaming binary to $@"
	@mv $^ $@

# tools
## valgrind memory leak detection
leak: $(BIND)/$(NAME)
	@echo "# running valgrind"
	rm -f valgrind.log
	cd $(BIND) && valgrind $(VALGRIND) 2> ../valgrind.log $(CMD)
	less valgrind.log
## repository cleaning
clean:
	@echo "# cleaning"
	rm -rf $(BIND) $(OBJD) valgrind.log
## running
run:
	@cd $(BIND) && $(CMD)
## remotes edition
remotes:
	@echo "# registering remotes"
	git remote add gitea ssh://git@git.nullgemm.fr:2999/nullgemm/$(NAME).git
	git remote add github git@github.com:nullgemm/$(NAME).git
	git remote remove origin
## submodules update
github:
	@echo "# sourcing submodules from https://github.com"
	cp .github .gitmodules
	git submodule sync
	git submodule update --init --remote
## submodules update
gitea:
	@echo "# sourcing submodules from personal server"
	cp .gitea .gitmodules
	git submodule sync
	git submodule update --init --remote
