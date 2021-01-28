#include "networking.h"

char **loadgopher(struct pageinfo *target) {
    /* empty magic string should be sent, thus
       make path empty if only char in path is / */
    if (!strcmp(target->path, "/")) {
        /* path is strdup'd / strcpy'd, so this
           should not cause any problems */
        target->path[0] = '\0';
    }

    /* declare request variables */
     char *magicstring = malloc(sizeof(char) * (strlen(target->path) + 3));

    /* build magic string (incl. \r\n) */
    strcpy(magicstring, target->path);
    strcat(magicstring, "\r\n");

    /* build socket vars & opts */
    int sockfd = -1, err, count;
    struct addrinfo opts = {}, *resolved, *addr;
    opts.ai_family = AF_UNSPEC; /* we want both IPv4 and IPv6 */
    opts.ai_socktype = SOCK_STREAM;
    opts.ai_protocol = IPPROTO_TCP;

    /* resolve host */
    err = getaddrinfo(target->host, target->port, &opts, &resolved);
    if (err != 0) {
        mvaddstr(LINES - 1, 0, "error: getaddrinfo: ");
        addstr(gai_strerror(err));
        refresh();
        return NULL;
    }
    
    /* create socket */
    for (addr = resolved; addr != NULL; addr = addr->ai_next) {
        sockfd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (sockfd == -1) {
            mvaddstr(LINES - 1, 0, "socket: ");
            addstr(strerror(errno));
            refresh();
            continue;
        }

        if (connect(sockfd, addr->ai_addr, addr->ai_addrlen) == 0) {
            break;
        } else {
            mvaddstr(LINES - 1, 0, "connect: ");
            addstr(strerror(errno));
            refresh();
        }

        close(sockfd);
    }

    /* free resolved stuff */
    freeaddrinfo(resolved);

    /* if no connecttion could be established, return NULL */
    if (sockfd == -1) {
        return NULL;
    }

    /* write magic number */
    count = write(sockfd, magicstring, strlen(magicstring));
    if (count < 0) {
        mvaddstr(LINES - 1, 0, "write: ");
        addstr(strerror(errno));
        refresh();
        return NULL;
    }

    /* prepare reading vars */
    int lines_alloc_current = 512, lines_alloc_step = 512, cline_alloc_current = 512, cline_alloc_step = 512, at_end = 0, current_line_idx = 0;
    char **lines = malloc(sizeof(char *) * lines_alloc_current);

    while (!at_end) {
        lines[current_line_idx] = malloc(sizeof(char) * cline_alloc_current);
        int current_alloc_increase = 0, cchar;
        
        for (cchar = 0; 1; cchar++) {
            char readc;
            if (!read(sockfd, &readc, 1)) {
                at_end = 1;
                break;
            }

            /* dynamically resize memory */
            if (cchar >= (cline_alloc_current + (cline_alloc_step * current_alloc_increase))) {
                current_alloc_increase++;
                lines[current_line_idx] = realloc(lines[current_line_idx], sizeof(char) * (cline_alloc_current + (cline_alloc_step * current_alloc_increase)));
            }

            /* if end of line, go to next line */
            if (readc == '\r' || readc == '\n') {
                lines[current_line_idx][cchar] = '\0';
                current_line_idx++;

                /* for files that send only \n */
                if (readc != '\n') {
                    read(sockfd, &readc, 1);
                    if (readc != '\n') {
                        /* if we'd have ignored a char, seek back by one */
                        lseek(sockfd, -1, SEEK_CUR);
                    }
                }

                break;
            }

            lines[current_line_idx][cchar] = readc;
        }

        /* dynamically resize memory */
        if (current_line_idx >= lines_alloc_current) {
            lines_alloc_current += lines_alloc_step;
            lines = realloc(lines, sizeof(char *) * lines_alloc_current);
        }

        if (!strcmp(lines[current_line_idx - 1], ".")) {
            at_end = 1;
        }

        /* write line count */
        target->linecount = current_line_idx;
    }

    /* close socket */
    close(sockfd);

    return lines;
}

void loadbinary(struct pageinfo *target, char *destpath) {
    /* open output file */
    int outfd = open(destpath, O_CREAT | O_WRONLY | O_TRUNC);
    if (outfd == -1) {
        mvaddstr(LINES - 1, 0, "open: ");
        addstr(strerror(errno));
        refresh();
        return;
    }

    /* declare request variables */
    char *magicstring = malloc(sizeof(char) * (strlen(target->path) + 3));

    /* build magic string (incl. \r\n) */
    strcpy(magicstring, target->path);
    strcat(magicstring, "\r\n");

    /* build socket vars & opts */
    int sockfd = -1, err, count;
    struct addrinfo opts = {}, *resolved, *addr;
    opts.ai_family = AF_UNSPEC; /* we want both IPv4 and IPv6 */
    opts.ai_socktype = SOCK_STREAM;
    opts.ai_protocol = IPPROTO_TCP;

    /* resolve host */
    err = getaddrinfo(target->host, target->port, &opts, &resolved);
    if (err != 0) {
        mvaddstr(LINES - 1, 0, "error: getaddrinfo: ");
        addstr(gai_strerror(err));
        refresh();
        return;
    }
    
    /* create socket */
    for (addr = resolved; addr != NULL; addr = addr->ai_next) {
        sockfd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (sockfd == -1) {
            mvaddstr(LINES - 1, 0, "socket: ");
            addstr(strerror(errno));
            refresh();
            continue;
        }

        if (connect(sockfd, addr->ai_addr, addr->ai_addrlen) == 0) {
            break;
        } else {
            mvaddstr(LINES - 1, 0, "connect: ");
            addstr(strerror(errno));
            refresh();
        }

        close(sockfd);
    }

    /* free resolved stuff */
    freeaddrinfo(resolved);

    /* if no connecttion could be established, return NULL */
    if (sockfd == -1) {
        return;
    }

    /* write magic number */
    count = write(sockfd, magicstring, strlen(magicstring));
    if (count < 0) {
        mvaddstr(LINES - 1, 0, "write: ");
        addstr(strerror(errno));
        refresh();
        return;
    }

    /* prepare reading vars */
    int bufsize = 1024, at_end = 0, errc = 0;
    char *buf = malloc(sizeof(char) * bufsize);

    while (!at_end && errc < 3) {
        ssize_t len = read(sockfd, buf, bufsize);
        if (len == 0) {
            at_end = 1;
        } else if (len == -1) {
            mvaddstr(LINES - 1, 0, "read: ");
            addstr(strerror(errno));
            refresh();
            errc++; /* only continue if <3 errors occured */
            continue;
        }

        ssize_t wrlen = write(outfd, buf, len);
        if (wrlen == -1) {
            mvaddstr(LINES - 1, 0, "write: ");
            addstr(strerror(errno));
            refresh();
            errc++; /* only continue if <3 errors occured */
            continue;
        }

        /* something interrupted our writing.
           if this happened, just try to write the rest */
        if (len != wrlen) {
            wrlen = write(outfd, buf + wrlen, len - wrlen);
            if (wrlen == -1) {
                mvaddstr(LINES - 1, 0, "write: ");
                addstr(strerror(errno));
                refresh();
                errc++; /* only continue if <3 errors occured */
                continue;
            }
        }
    }

    /* close socket and output file */
    close(sockfd);
    close(outfd);
}

struct pageinfo *parseurl(char *url) {
    int urllen = strlen(url), i = 1;

    struct pageinfo *parsedurl = malloc(sizeof(struct pageinfo));

    /* parseurl modifies the original url param, thus we duplicate
       the passed url at the beginning */
    parsedurl->url = strdup(url);

    /* find scheme */
    for (; i < urllen - 2; i++) {
        if (url[i] == ':' && url[i + 1] == '/' && url[i + 2] == '/') {
            url[i] = '\0';
            parsedurl->scheme = url;
            parsedurl->host = url + i + 3;
            break;
        }
    }
    if (i == urllen - 2) { /* if no scheme specified */
        parsedurl->scheme = strdup("gopher");
        i = 0;
        parsedurl->host = url;
    }
    i += 3;

    /* find hostname */
    for (; i < urllen; i++) {
        if (url[i] == ':') { /* end of hostname, port begin */
            url[i] = '\0';
            parsedurl->port = url + i + 1;

            /* search for path */
            for (i++; i < urllen; i++) {
                if (url[i] == '/') {
                    parsedurl->path = malloc(sizeof(char) * (urllen - i + 1));
                    strcpy(parsedurl->path, url + i);
                    url[i] = '\0';

                    break;
                }
            }

            if (i == urllen) {
                parsedurl->path = strdup("/");
            }

            break;
        } else if (url[i] == '/') { /* end of hostname, path begin */
            parsedurl->path = malloc(sizeof(char) * (urllen - i + 1));
            strcpy(parsedurl->path, url + i); /* preserve leading slash */
            url[i] = '\0';

            /* find default/standard port value */
            if (!strcmp(parsedurl->scheme, "gopher")) {
                parsedurl->port = strdup("70");
            } else if (!strcmp(parsedurl->scheme, "https")) {
                parsedurl->port = strdup("443");
            } else {
                parsedurl->port = strdup("80");
            }

            break;
        }
    }
    if (i == urllen) {
        /* find default/standard port value */
        if (!strcmp(parsedurl->scheme, "gopher")) {
            parsedurl->port = strdup("70");
        } else if (!strcmp(parsedurl->scheme, "https")) {
            parsedurl->port = strdup("443");
        } else {
            parsedurl->port = strdup("80");
        }
            
        parsedurl->path = strdup("/");
    }

    return parsedurl;
}

struct simplepage *handleloadrequest(char *url) {
    struct pageinfo *urlparts = parseurl(url);

    /* title isn't used, saves a few bytes */
    /* urlparts->title = strdup("gopher"); */

    char **lines = loadgopher(urlparts);
    if (lines == NULL) {
        return NULL;
    }

    struct simplepage *parsedfinal = parsegopher(lines, urlparts);

    return parsedfinal;
}

struct simplepage *followlink(struct simplepage *current, unsigned int linum) {
    struct pageinfo *meta = malloc(sizeof(struct pageinfo));

    *meta = (struct pageinfo) {
        .scheme = current->meta->scheme, /* followlink() stays on gopher protocol */
        .host = current->lines[linum]->host,
        .port = current->lines[linum]->port,
        .path = current->lines[linum]->magicString,
        .url = current->lines[linum]->host, /* TODO: construct fancy url */
        .title = NULL, /* title isn't used */
        .linecount = 0
    };

    char **lines = loadgopher(meta);
    if (lines == NULL) {
        return NULL;
    }

    struct simplepage *parsedfinal = parsegopher(lines, meta);

    return parsedfinal;
}

struct simplepage *followprompt(struct simplepage *current, unsigned int linum, char *query) {
    struct pageinfo *meta = malloc(sizeof(struct pageinfo));

    char *pathquery = malloc(sizeof(char) * (strlen(current->lines[linum]->magicString) + strlen(query) + 3));
    strcpy(pathquery, current->lines[linum]->magicString);
    strcat(pathquery, "\t");
    strcat(pathquery, query);
    
    *meta = (struct pageinfo) {
        .scheme = current->meta->scheme, /* followlink() stays on gopher protocol */
        .host = current->lines[linum]->host,
        .port = current->lines[linum]->port,
        .path = pathquery,
        .url = current->lines[linum]->host, /* TODO: construct fancy url */
        .title = NULL, /* title isn't used */
        .linecount = 0
    };

    char **lines = loadgopher(meta);
    if (lines == NULL) {
        return NULL;
    }

    struct simplepage *parsedfinal = parsegopher(lines, meta);

    return parsedfinal;
}

struct simplepage *followplain(struct simplepage *current, unsigned int linum) {
    struct pageinfo *meta = malloc(sizeof(struct pageinfo));

    *meta = (struct pageinfo) {
        .scheme = current->meta->scheme, /* followlink() stays on gopher protocol */
        .host = current->lines[linum]->host,
        .port = current->lines[linum]->port,
        .path = current->lines[linum]->magicString,
        .url = current->lines[linum]->host, /* TODO: construct fancy url */
        .title = NULL, /* title isn't used */
        .linecount = 0
    };

    char **lines = loadgopher(meta);
    if (lines == NULL) {
        return NULL;
    }

    struct simplepage *parsedfinal = parseplain(lines, meta);

    /* fix for final dot being rendered */
    parsedfinal->meta->linecount--;
    
    return parsedfinal;
}

struct simplepage *followhyper(struct simplepage *current, unsigned int linum) {
    /* find browser from env, otherwise use xdg-open */
    char *browser = getenv("BROWSER"), *url;
    if (!browser) {
        browser = strdup("xdg-open");
    }
    
    switch (fork()) {
        case 0:
            url = current->lines[linum]->magicString;
            if (url[0] == '/') {
                url++; /* skip leading slash */
            }
            if (!strncmp(url, "URL:", 4)) {
                url += 4;
            }

            char *const runcmd[] = { browser, url, NULL };

            execvp(runcmd[0], runcmd);

            perror("execvp");
            _exit(1);
        case -1:
            mvaddstr(LINES - 1, 0, "fork: ");
            addstr(strerror(errno));
            refresh();
            return NULL;
        default:
            return NULL;
    }
}

struct simplepage *followbinary(struct simplepage *current, unsigned int linum, char *dest) {
    switch (fork()) {
        case 0:
            mvaddstr(LINES - 1, 0, "downloading ");
            addstr(dest);
            addstr("...");
            refresh();

            struct pageinfo *meta = malloc(sizeof(struct pageinfo));

            *meta = (struct pageinfo) {
                .scheme = current->meta->scheme,
                .host = current->lines[linum]->host,
                .port = current->lines[linum]->port,
                .path = current->lines[linum]->magicString,
                .url = current->lines[linum]->host,
                .title = NULL, /* title isn't used */
                .linecount = 0
            };

            loadbinary(meta, dest);

            _exit(0);
        case -1:
            mvaddstr(LINES - 1, 0, "fork: ");
            addstr(strerror(errno));
            refresh();
            return NULL;
        default:
            return NULL;
    }
}
