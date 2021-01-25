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
                fprintf(stderr, "error: page %s ( at %s ) contains unknown line type %c!\n", metadata->path, metadata->url, responsetext[i][0]);
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
        /* in plain text, everything is of type information */
        enum linetype ltype = information;

        struct typedline *cline = malloc(sizeof(struct typedline));

        cline->ltype = ltype;
        cline->text = responsetext[i] + 1; /* pop first char */

        char *empty = malloc(sizeof(char));
        *empty = '\0';
        cline->magicString = empty;
        cline->host = empty;
        cline->port = empty;

        lines[i] = cline;
    }

    return parsedpage;
}
