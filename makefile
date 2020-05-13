NAME = globox
CC = gcc
FLAGS = -std=c99 -pedantic -g
FLAGS+= -Wall -Wextra -Werror=vla -Werror -Wno-unused-parameter 
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
BACKEND ?= quartz

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
endif
ifeq ($(BACKEND), wayland)
LINK+= -lxkbcommon
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
SRCS+= $(SUBD)/willis/src/x11.c
SRCS+= $(SUBD)/willis/src/xkb.c
SRCS+= $(SUBD)/willis/src/debug.c
INCL+= -I$(SUBD)/willis/src
endif
endif

## wayland
ifeq ($(BACKEND), wayland)
FLAGS+= -DGLOBOX_WAYLAND
SRCS+= $(SRCD)/nix.c
SRCS+= $(SRCD)/wayland.c
SRCS+= $(SRCD)/globox_wayland.c
SRCS+= $(INCD)/xdg-shell-protocol.c
LINK+= -lwayland-client -lrt
.PHONY: final
final: | $(INCD) $(BIND)/$(NAME)

ifeq ($(EXAMPLE), willis)
FLAGS+= -DWILLIS_DEBUG
FLAGS+= -DWILLIS_WAYLAND
SRCS+= $(SUBD)/willis/src/wayland.c
SRCS+= $(SUBD)/willis/src/xkb.c
SRCS+= $(SUBD)/willis/src/debug.c
INCL+= -I$(SUBD)/willis/src
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
SRCS+= $(SUBD)/willis/src/win.c
SRCS+= $(SUBD)/willis/src/debug.c
INCL+= -I$(SUBD)/willis/src
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
SRCS+= $(SUBD)/willis/src/quartz.c
SRCS+= $(SUBD)/willis/src/debug.c
INCL+= -I$(SUBD)/willis/src
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
	@git remote add github git@github.com:cylgom/$(NAME).git
	@git remote add gitea ssh://git@git.cylgom.net:2999/cylgom/$(NAME).git

github:
	@echo "sourcing submodules from https://github.com"
	@cp .github .gitmodules
	@git submodule sync
	@git submodule update --init --remote

gitea:
	@echo "sourcing submodules from https://git.cylgom.net"
	@cp .gitea .gitmodules
	@git submodule sync
	@git submodule update --init --remote
