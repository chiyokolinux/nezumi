# this file is part of nezumi
# Copyright (c) 2021 Emily <elishikawa@jagudev.net>
#
# nezumi is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# nezumi is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with nezumi.  If not, see <https://www.gnu.org/licenses/>.

VERSION = 1.5
NAME	= nezumi

PREFIX = /usr
MANPREFIX = $(PREFIX)/share/man

CC = gcc
LD = $(CC)
CPPFLAGS =
CFLAGS   = -Wextra -Wall -Os -std=gnu89
LDFLAGS  = -s
LDLIBS   = -lncurses
