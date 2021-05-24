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

#ifndef PARSER_H
#define PARSER_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <curses.h>

enum linetype {
    file,
    directory,
    csoserver,
    error,
    binhex,
    dosbinary,
    uuencoded,
    indexserver,
    telnetsession,
    binaryfile,
    duplicate,
    gifimage,
    image,
    tn3270session,
    information,
    hyperlink,
    document,
    soundfile,
    bookmark
};

struct typedline {
    enum linetype ltype;
    char *text;
    char *magicString;
    char *host;
    char *port;
};

struct pageinfo {
    char *scheme;
    char *host;
    char *port;
    char *path;
    char *url;
    char *title;
    unsigned int linecount;
};

struct simplepage {
    struct typedline **lines;
    struct pageinfo *meta;
};

struct bookmark_list {
    int length;
    int alloc_length;
    struct bookmark **bookmarks;
};

struct bookmark {
    char name[255];
    char url[1023];
};

struct simplepage *parsegopher(char **responsetext, struct pageinfo *metadata);
struct simplepage *parseplain(char **responsetext, struct pageinfo *metadata);
void freesimplepage(struct simplepage *to_free, int free_struct_itself);

#endif /* PARSER_H */
