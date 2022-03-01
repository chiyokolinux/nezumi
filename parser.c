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

#include "parser.h"

struct simplepage *parsegopher(char **responsetext, struct pageinfo *metadata) {
    struct typedline **lines = malloc(sizeof(struct typedline *) * metadata->linecount);
    struct simplepage *parsedpage = malloc(sizeof(struct simplepage));
    *parsedpage = (struct simplepage){ .lines = lines, .meta = metadata };
    unsigned int i;

    for (i = 0; i < metadata->linecount; i++) {
        /* parse type */
        enum linetype ltype = error;
        switch (responsetext[i][0]) {
            case 'i':
                ltype = information;
                break;
            case '0':
                ltype = file;
                break;
            case '1':
                ltype = directory;
                break;
            case '2':
                ltype = csoserver;
                break;
            case '3':
                ltype = error;
                break;
            case '4':
                ltype = binhex;
                break;
            case '5':
                ltype = dosbinary;
                break;
            case '6':
                ltype = uuencoded;
                break;
            case '7':
                ltype = indexserver;
                break;
            case '8':
                ltype = telnetsession;
                break;
            case '9':
                ltype = binaryfile;
                break;
            case '+':
                ltype = duplicate;
                break;
            case 'g':
                ltype = gifimage;
                break;
            case 'I':
                ltype = image;
                break;
            case 'T':
                ltype = tn3270session;
                break;
            case 'd':
                ltype = document;
                break;
            case 'h':
                ltype = hyperlink;
                break;
            case 's':
                ltype = soundfile;
                break;
            default:
                mvaddstr(LINES - 1, 0, "error: page ");
                addstr(metadata->path);
                addstr(" ( at ");
                addstr(metadata->url);
                addstr(" ) contains unknown line type '");
                addch(responsetext[i][0]);
                addstr("'!");
                refresh();
                ltype = error;
                break;
        }

        struct typedline *cline = malloc(sizeof(struct typedline));

        cline->ltype = ltype;
        cline->text = responsetext[i] + 1; /* pop first char */

        unsigned int linelen = strlen(responsetext[i]), pos;
        int currentval = 0;
        for (pos = 1; pos < linelen; pos++) {
            if (responsetext[i][pos] == '\t') {
                responsetext[i][pos] = '\0';
                if (currentval == 0) {
                    cline->magicString = responsetext[i] + pos + 1;
                } else if (currentval == 1) {
                    cline->host = responsetext[i] + pos + 1;
                } else if (currentval == 2) {
                    cline->port = responsetext[i] + pos + 1;
                    break;
                }
                currentval++;
            }
        }

        lines[i] = cline;
    }

    return parsedpage;
}

struct simplepage *parseplain(char **responsetext, struct pageinfo *metadata) {
    struct typedline **lines = malloc(sizeof(struct typedline *) * metadata->linecount);
    struct simplepage *parsedpage = malloc(sizeof(struct simplepage));
    *parsedpage = (struct simplepage){ .lines = lines, .meta = metadata };
    unsigned int i;

    for (i = 0; i < metadata->linecount; i++) {
        /* in plain text, everything is of type plain */
        enum linetype ltype = plain;

        struct typedline *cline = malloc(sizeof(struct typedline));

        cline->ltype = ltype;
        cline->text = responsetext[i];

        char *empty = malloc(sizeof(char));
        *empty = '\0';
        cline->magicString = empty;
        cline->host = empty;
        cline->port = empty;

        lines[i] = cline;
    }

    return parsedpage;
}

struct simplepage *genbookmarkspage(struct bookmark_list *bml) {
    struct typedline **lines = malloc(sizeof(struct typedline *) * bml->length);
    struct simplepage *parsedpage = malloc(sizeof(struct simplepage));
    struct pageinfo *meta = malloc(sizeof(struct pageinfo));
    *meta = (struct pageinfo) {
        .scheme = strdup("nezumi"),
        .host = strdup("bookmarks"),
        .port = strdup("0"),
        .path = strdup("/all"),
        .url = strdup("nezumi://bookmarks:0/all"),
        .title = NULL,
        .linecount = bml->length
    };
    *parsedpage = (struct simplepage){ .lines = lines, .meta = meta };
    unsigned int i;

    for (i = 0; i < meta->linecount; i++) {
        enum linetype ltype = bookmark;

        struct typedline *cline = malloc(sizeof(struct typedline));

        cline->ltype = ltype;
        cline->text = bml->bookmarks[i]->name;

        char *empty = malloc(sizeof(char));
        *empty = '\0';
        cline->magicString = empty;
        cline->host = bml->bookmarks[i]->url;
        cline->port = empty;

        lines[i] = cline;
    }

    return parsedpage;
}

void freesimplepage(struct simplepage *to_free, int free_struct_itself) {
    /* networking.c takes metadata from lines of the
       previous history entry, so no free-ing required */

    /* free lines */
    unsigned int i;
    for (i = 0; i < to_free->meta->linecount; i++) {
        if (to_free->lines[i]->ltype != bookmark) /* DO NOT MESS WITH OUR BOOKMARKS! */
            /* ltype information is no longer assigned to lines parsed from plain text
               files as they do not have a type char before the content start that needs
               to be free()'d as well, while gopher information lines do. */
            free(to_free->lines[i]->text - (to_free->lines[i]->ltype != plain));
    }

    /* free top-level struct data */
    free(to_free->lines);
    free(to_free->meta);
    if (free_struct_itself) {
        free(to_free);
    }
}
