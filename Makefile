#
# SPDX-License-Identifier: ISC
#
# Copyright (c) 2022 Codethink
#

all: cdt

BUILDDIR := build

CC ?= gcc
MKDIR ?= mkdir -p
PKG_CONFIG ?= pkg-config

CPPFLAGS += -MMD -MP
CFLAGS += -Isrc
CFLAGS += -g -O0 -std=c11 -D_GNU_SOURCE
CFLAGS += -Wall -Wextra -pedantic -Wconversion -Wwrite-strings -Wcast-align \
		-Wpointer-arith -Winit-self -Wshadow -Wstrict-prototypes \
		-Wmissing-prototypes -Wredundant-decls -Wundef -Wvla \
		-Wdeclaration-after-statement

PKG_DEPS := libwebsockets libcyaml sdl2 SDL2_image
CFLAGS += $(shell $(PKG_CONFIG) --cflags $(PKG_DEPS))
LDFLAGS += $(shell $(PKG_CONFIG) --libs $(PKG_DEPS))

SRC := $(addprefix src/,cdt.c display.c)
SRC += $(addprefix src/cmd/,cmd.c)
SRC += $(addprefix src/msg/,msg.c queue.c)
SRC += $(addprefix src/util/,base64.c buffer.c cli.c file.c log.c)
SRC += $(shell find src/cmd/handler -type f -name *.c)
SRC += $(shell find src/msg/handler -type f -name *.c)
OBJ := $(patsubst %.c,%.o, $(addprefix $(BUILDDIR)/,$(SRC)))
DEP := $(patsubst %.c,%.d, $(addprefix $(BUILDDIR)/,$(SRC)))

$(OBJ): $(BUILDDIR)/%.o : %.c
	$(Q)$(MKDIR) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

cdt: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(BUILDDIR)

-include $(DEP)

.PHONY: all clean install
