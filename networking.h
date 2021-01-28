#ifndef NETWORKING_H
#define NETWORKING_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

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

#endif /* NETWORKING_H */
