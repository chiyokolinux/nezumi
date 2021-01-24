VERSION = 0.1
NAME	= nezumi

PREFIX = /usr
MANPREFIX = $(PREFIX)/share/man

CC = gcc
LD = $(CC)
CPPFLAGS =
CFLAGS   = -Wextra -Wall -Os -g
LDFLAGS  = # -s
LDLIBS   = -lncurses
