NAME = globox
CC = gcc
FLAGS = -std=c99 -pedantic -g
FLAGS+= -Wall -Wno-unused-parameter -Wextra -Werror=vla -Werror
FLAGS+= -DGLOBOX_X11
#FLAGS+= -DGLOBOX_WAYLAND
VALGRIND = --show-leak-kinds=all --track-origins=yes --leak-check=full
LINK = -lxcb -lxcb-shm -lxcb-randr
#LINK+= -lwayland-client -lrt
CMD = ./$(NAME)

BIND = bin
OBJD = obj
SRCD = src
INCD = inc
SUBD = sub
RESD = res

INCL = -I$(SRCD)
INCL+= -I$(INCD)

SRCS = $(SRCD)/main.c
SRCS+= $(SRCD)/globox.c
SRCS+= $(SRCD)/globox_x11.c
#SRCS+= $(SRCD)/globox_wayland.c
#SRCS+= $(INCD)/xdg-shell-protocol.c

SRCS_OBJS := $(patsubst %.c,$(OBJD)/%.o,$(SRCS))
SRCS_OBJS += $(OBJD)/$(RESD)/iconpix.o

# aliases
.PHONY: final
final: $(INCD) $(BIND)/$(NAME)

$(INCD):
	@echo "generating wayland protocol extensions source files"
	@mkdir $@
	@wayland-scanner private-code \
	< /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml \
	> $@/xdg-shell-protocol.c
	@wayland-scanner client-header \
	< /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml \
	> $@/xdg-shell-client-protocol.h

$(RESD)/iconpix.bin:
	@echo "generating icons pixmap"
	@cd $(RESD) && ./makepix.sh

$(OBJD)/$(RESD)/iconpix.o: $(RESD)/iconpix.bin
	@echo "building icon object"
	@mkdir -p $(@D)
	@objcopy -I binary -O elf64-x86-64 -B i386:x86-64 \
	--redefine-syms=$(RESD)/syms.map \
	--rename-section .data=.iconpix \
	$< $@

# generic compiling command
$(OBJD)/%.o: %.c
	@echo "building object $@"
	@mkdir -p $(@D)
	@$(CC) $(INCL) $(FLAGS) -c -o $@ $<

# final executable
$(BIND)/$(NAME): $(SRCS_OBJS) $(FINAL_OBJS)
	@echo "compiling executable $@"
	@mkdir -p $(@D)
	@$(CC) -o $@ $^ $(LINK)

run:
	@cd $(BIND) && $(CMD)

# tools
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
