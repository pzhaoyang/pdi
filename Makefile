CC = gcc
RM = rm -rf

DEBUG = y
CFLAGS := -Wall

ifeq ($(DEBUG), y)
CFLAGS += -g
else
CFLAGS += -O2
endif

SUBDIRS := client \
			server \
			debug_tool \
			socket



src := client.c
hdr := include

.PHONY: debug_tool

debug_tool: $(src) $(hdr)

.PHONY: clean
clean:
	@echo remove all generate files