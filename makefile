NAME = globox
#CC = gcc
CC = x86_64-w64-mingw32-gcc
FLAGS = -std=c99 -pedantic -g
FLAGS+= -Wall -Wno-unused-parameter -Wextra -Werror=vla -Werror
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

RENDER ?= swr
BACKEND ?= win

# rendering backends
## software
ifeq ($(RENDER), swr)
FLAGS+= -DGLOBOX_RENDER_SWR
SRCS = $(SRCD)/main_epoll.c
endif

## vulkan
ifeq ($(RENDER), vlk)
FLAGS+= -DGLOBOX_RENDER_VLK
LINK+= -lvulkan
LINK+= -lVkLayer_khronos_validation
SRCS = $(SRCD)/main_vulkan.c
endif

## opengl
ifeq ($(RENDER), ogl)
FLAGS+= -DGLOBOX_RENDER_OGL
LINK+= -lX11 -lX11-xcb -lGL
SRCS = $(SRCD)/main_callback.c
endif

# windowing backends
## x11
ifeq ($(BACKEND), x11)
FLAGS+= -DGLOBOX_X11
SRCS+= $(SRCD)/x11.c
SRCS+= $(SRCD)/globox_x11.c
LINK+= -lxcb -lxcb-shm -lxcb-randr -lrt
.PHONY: final
final: $(BIND)/$(NAME)
endif

## wayland
ifeq ($(BACKEND), wayland)
FLAGS+= -DGLOBOX_WAYLAND
SRCS+= $(SRCD)/wayland.c
SRCS+= $(SRCD)/globox_wayland.c
SRCS+= $(INCD)/xdg-shell-protocol.c
LINK = -lwayland-client -lrt
.PHONY: final
final: | $(INCD) $(BIND)/$(NAME)
endif

## win
ifeq ($(BACKEND), win)
FLAGS+= -DGLOBOX_WIN -DUNICODE -D_UNICODE
SRCS = $(SRCD)/main_win_getmessage.c
SRCS+= $(SRCD)/win.c
SRCS+= $(SRCD)/globox_win.c
LINK+= -lgdi32 -mwindows
CMD = wine ./$(NAME)
.PHONY: final
final: $(BIND)/$(NAME)
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
