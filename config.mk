VERSION = 1.0
NAME	= nezumi

PREFIX = /usr
MANPREFIX = $(PREFIX)/share/man

CC = gcc
LD = $(CC)
CPPFLAGS =
CFLAGS   = -Wextra -Wall -Os -std=gnu89
LDFLAGS  = -s
LDLIBS   = -lncurses
