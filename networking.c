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
    struct addrinfo opts = {}, *resolved;
    opts.ai_family = AF_UNSPEC; /* we want both IPv4 and IPv6 */
    opts.ai_socktype = SOCK_STREAM;
    opts.ai_protocol = IPPROTO_TCP;

    /* resolve host */
    err = getaddrinfo(target->host, target->port, &opts, &resolved);
    if (err != 0) {
        fprintf(stderr, "error: getaddrinfo: %s", gai_strerror(err));
        return NULL;
    }
    
    /* create socket */
    for(struct addrinfo *addr = resolved; addr != NULL; addr = addr->ai_next) {
        sockfd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (sockfd == -1) {
            perror("socket");
            continue;
        }

        if (connect(sockfd, addr->ai_addr, addr->ai_addrlen) == 0) {
            break;
        } else {
            perror("connect");
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
        perror("write");
        return NULL;
    }

    /* prepare reading vars */
    int lines_alloc_current = 512, lines_alloc_step = 512, cline_alloc_current = 512, cline_alloc_step = 512, at_end = 0, current_line_idx = 0;
    char **lines = malloc(sizeof(char *) * lines_alloc_current);

    while (!at_end) {
        lines[current_line_idx] = malloc(sizeof(char) * cline_alloc_current);
        int current_alloc_increase = 0;
        
        for (int cchar = 0; 1; cchar++) {
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
                read(sockfd, &readc, 1);
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
        target->linecount = current_line_idx - 2;
    }

    /* close socket */
    close(sockfd);

    return lines;
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
    if (i == urllen) { /* if no scheme specified */
        parsedurl->scheme = strdup("gopher");
        i = 0;
        parsedurl->host = url;
    }

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
                }
            }
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
        }
    }

    return parsedurl;
}

struct simplepage *handleloadrequest(char *url) {
    struct pageinfo *urlparts = parseurl(url);

    urlparts->title = strdup("gopher");

    char **lines = loadgopher(urlparts);

    struct simplepage *parsedfinal = parsegopher(lines, urlparts);

    return parsedfinal;
}
