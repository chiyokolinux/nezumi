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
    soundfile
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

struct simplepage *parsegopher(char **responsetext, struct pageinfo *metadata);
struct simplepage *parseplain(char **responsetext, struct pageinfo *metadata);

#endif /* PARSER_H */
