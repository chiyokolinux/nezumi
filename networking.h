/**
 * this file is part of nezumi
 * Copyright (c) 2021 Emily <elishikawa@jagudev.net>
 *
 * nezumi is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * nezumi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with nezumi.  If not, see <https://www.gnu.org/licenses/>.
**/

#ifndef NETWORKING_H
#define NETWORKING_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <curses.h>

#include "parser.h"

char **loadgopher(struct pageinfo *target);
struct pageinfo *parseurl(char *url);
struct simplepage *handleloadrequest(char *url);
struct simplepage *followlink(struct simplepage *current, unsigned int linum);
struct simplepage *followprompt(struct simplepage *current, unsigned int linum, char *query);
struct simplepage *followplain(struct simplepage *current, unsigned int linum);
struct simplepage *followhyper(struct simplepage *current, unsigned int linum);
struct simplepage *followbinary(struct simplepage *current, unsigned int linum, char *dest);

#endif /* NETWORKING_H */
