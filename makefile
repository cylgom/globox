NAME = globox
CC = gcc
FLAGS = -std=c99 -pedantic -g
FLAGS+= -Wall -Wextra -Werror=vla -Werror -Wno-unused-parameter -Wno-address-of-packed-member
VALGRIND = --show-leak-kinds=all --track-origins=yes --leak-check=full --suppressions=../res/valgrind.supp
CMD = ./$(NAME)

BIND = bin
OBJD = obj
SRCD = src
INCD = inc
SUBD = sub
RESD = res

INCL = -I$(SRCD)
INCL+= -I$(INCD)
SRCS =
SRCS_OBJS = $(OBJD)/$(RESD)/icon/iconpix.o
LINK =

EXAMPLE ?= willis
BACKEND ?= wayland

# rendering backends
## software
ifeq ($(EXAMPLE), swr)
FLAGS+= -DGLOBOX_RENDER_SWR
SRCS = $(SRCD)/main.c
endif

## vulkan
ifeq ($(EXAMPLE), vlk)
FLAGS+= -DGLOBOX_RENDER_VLK
LINK+= -lvulkan
LINK+= -lVkLayer_khronos_validation
SRCS = $(SRCD)/main_vulkan.c
endif

## opengl
ifeq ($(EXAMPLE), ogl)
FLAGS+= -DGLOBOX_RENDER_OGL
LINK+= -lX11 -lX11-xcb -lGL
SRCS = $(SRCD)/main_opengl.c
endif

## willis
ifeq ($(EXAMPLE), willis)
FLAGS+= -DGLOBOX_RENDER_SWR
SRCS = $(SRCD)/main_willis.c
ifeq ($(BACKEND), x11)
LINK+= -lxkbcommon-x11
LINK+= -lxkbcommon
LINK+= -lxcb-xkb
LINK+= -lxcb-xinput
LINK+= -lxcb-xfixes
LINK+= -lxcb-cursor
LINK+= -lxcb-randr
LINK+= -lxcb-xrm
endif
ifeq ($(BACKEND), wayland)
LINK+= -lxkbcommon
LINK+= -lwayland-cursor
endif
endif

# windowing backends
## x11
ifeq ($(BACKEND), x11)
FLAGS+= -DGLOBOX_X11
SRCS+= $(SRCD)/nix.c
SRCS+= $(SRCD)/x11.c
SRCS+= $(SRCD)/globox_x11.c
LINK+= -lxcb -lxcb-shm -lxcb-randr -lrt
.PHONY: final
final: $(BIND)/$(NAME)

ifeq ($(EXAMPLE), willis)
FLAGS+= -DWILLIS_DEBUG
FLAGS+= -DWILLIS_X11
FLAGS+= -DCURSORYX_X11
FLAGS+= -DDPISHIT_X11
SRCS+= $(SUBD)/willis/src/x11.c
SRCS+= $(SUBD)/willis/src/xkb.c
SRCS+= $(SUBD)/willis/src/debug.c
SRCS+= $(SUBD)/cursoryx/src/x11.c
SRCS+= $(SUBD)/dpishit/src/x11.c
SRCS+= $(SUBD)/dpishit/src/nix.c
INCL+= -I$(SUBD)/willis/src
INCL+= -I$(SUBD)/cursoryx/src
INCL+= -I$(SUBD)/dpishit/src
endif
endif

## wayland
ifeq ($(BACKEND), wayland)
FLAGS+= -DGLOBOX_WAYLAND
SRCS+= $(SRCD)/nix.c
SRCS+= $(SRCD)/wayland.c
SRCS+= $(SRCD)/globox_wayland.c
SRCS+= $(INCD)/xdg-shell-protocol.c
SRCS+= $(INCD)/zwp-relative-pointer-protocol.c
SRCS+= $(INCD)/zwp-pointer-constraints-protocol.c
LINK+= -lwayland-client -lrt
.PHONY: final
final: | $(INCD) $(BIND)/$(NAME)

ifeq ($(EXAMPLE), willis)
FLAGS+= -DWILLIS_DEBUG
FLAGS+= -DWILLIS_WAYLAND
FLAGS+= -DCURSORYX_WAYLAND
FLAGS+= -DDPISHIT_WAYLAND
SRCS+= $(SUBD)/willis/src/wayland.c
SRCS+= $(SUBD)/willis/src/xkb.c
SRCS+= $(SUBD)/willis/src/debug.c
SRCS+= $(SUBD)/cursoryx/src/wayland.c
SRCS+= $(SUBD)/dpishit/src/wayland.c
SRCS+= $(SUBD)/dpishit/src/nix.c
INCL+= -I$(SUBD)/willis/src
INCL+= -I$(SUBD)/cursoryx/src
INCL+= -I$(SUBD)/dpishit/src
LINK+= -lpthread
endif
endif

## win
ifeq ($(BACKEND), win)
CC = x86_64-w64-mingw32-gcc
FLAGS+= -DGLOBOX_WIN -DUNICODE -D_UNICODE
SRCS+= $(SRCD)/win.c
SRCS+= $(SRCD)/globox_win.c
LINK+= -lgdi32 -mwindows
CMD = wine ./$(NAME)
.PHONY: final
final: $(BIND)/$(NAME)

ifeq ($(EXAMPLE), willis)
FLAGS+= -DWILLIS_DEBUG
FLAGS+= -DWILLIS_WIN
FLAGS+= -DCURSORYX_WIN
FLAGS+= -DDPISHIT_WIN
FLAGS+= -DNTDDI_VERSION=NTDDI_WINBLUE
SRCS+= $(SUBD)/willis/src/win.c
SRCS+= $(SUBD)/willis/src/debug.c
SRCS+= $(SUBD)/cursoryx/src/win.c
SRCS+= $(SUBD)/dpishit/src/win.c
INCL+= -I$(SUBD)/willis/src
INCL+= -I$(SUBD)/cursoryx/src
INCL+= -I$(SUBD)/dpishit/src
LINK+= -lshcore
endif
endif

## quartz
ifeq ($(BACKEND), quartz)
CC = o64-clang
FLAGS+= -DGLOBOX_QUARTZ
SRCS+= $(SRCD)/quartz.c
SRCS+= $(SRCD)/quartz_helpers.c
SRCS+= $(SRCD)/globox_quartz.c
SRCS_OBJS = $(OBJD)/$(RESD)/icon/iconpix_mach.o
LINK+= -framework AppKit
#CMD = wine ./$(NAME)
.PHONY: final
final: $(BIND)/$(NAME)

ifeq ($(EXAMPLE), willis)
FLAGS+= -DWILLIS_DEBUG
FLAGS+= -DWILLIS_QUARTZ
FLAGS+= -DCURSORYX_QUARTZ
FLAGS+= -DDPISHIT_OSX
SRCS+= $(SUBD)/willis/src/quartz.c
SRCS+= $(SUBD)/willis/src/debug.c
SRCS+= $(SUBD)/cursoryx/src/quartz.c
SRCS+= $(SUBD)/dpishit/src/osx.c
INCL+= -I$(SUBD)/willis/src
INCL+= -I$(SUBD)/cursoryx/src
INCL+= -I$(SUBD)/dpishit/src
endif
endif

# rest of the makefile
## objects listing
SRCS_OBJS+= $(patsubst %.c,$(OBJD)/%.o,$(SRCS))

## wayland
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

## icon binary
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

$(OBJD)/$(RESD)/icon/iconpix_mach.o: $(OBJD)/$(RESD)/icon/iconpix.o
	@echo "converting icon object to mach-o ABI"
	@objconv -fmac64 -nu+ -v0 $< $@

## generic compiling command
$(OBJD)/%.o: %.c
	@echo "building object $@"
	@mkdir -p $(@D)
	@$(CC) $(INCL) $(FLAGS) -c -o $@ $<

## final executable
$(BIND)/$(NAME): $(SRCS_OBJS)
	@echo "compiling executable $@"
	@mkdir -p $(@D)
	@$(CC) -o $@ $^ $(LINK)

run:
	@cd $(BIND) && $(CMD)

## tools
leak: leakgrind
leakgrind: $(BIND)/$(NAME)
	@rm -f valgrind.log
	@cd $(BIND) && valgrind $(VALGRIND) 2> ../valgrind.log $(CMD)
	@less valgrind.log

clean:
	@echo "cleaning"
	@rm -rf $(BIND) $(OBJD) valgrind.log

remotes:
	@echo "registering remotes"
	@git remote add github git@github.com:nullgemm/$(NAME).git
	@git remote add gitea ssh://git@git.nullgemm.fr:2999/nullgemm/$(NAME).git

github:
	@echo "sourcing submodules from https://github.com"
	@cp .github .gitmodules
	@git submodule sync
	@git submodule update --init --remote

gitea:
	@echo "sourcing submodules from personal server"
	@cp .gitea .gitmodules
	@git submodule sync
	@git submodule update --init --remote
