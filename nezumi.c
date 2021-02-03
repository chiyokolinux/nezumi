#include <stdio.h>
#include <string.h>
#include <signal.h>

#include <curses.h>

#include "config.h"
#include "networking.h"
#include "parser.h"

void init_colors();
void set_header_text(char text[]);
void load_page(char *url);
void mainloop();
void prompt_url();
void prompt_index(unsigned int linum);
void prompt_download(unsigned int linum);
void scroll_current(unsigned int factor);
void hist_prev();
void hist_next();

struct simplepage *currentsite = NULL;
struct simplepage **history = NULL;
int histidx = -1, histmax = 0;

int main(int argc, char **argv) {
    /* init curses */
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, true);
    scrollok(stdscr, true);
    wclear(stdscr);
    start_color();
    init_colors();

    /* instantly reap children, do not reap them in some fancy handler */
    signal(SIGCHLD, SIG_IGN);

    /* malloc history */
    history = malloc(sizeof(struct simplepage *) * HISTSIZE);

    set_header_text("welcome to nezumi!");

    if (argc > 1) {
        load_page(strdup(argv[1]));
    } else {
        load_page(strdup("gopher://gopher.floodgap.com/"));
    }

    mainloop();

    /* end curses */
    nocbreak();
    keypad(stdscr, false);
    echo();
    endwin();

    return 0;
}

void init_colors() {
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_CYAN, COLOR_BLACK);
    init_pair(4, COLOR_WHITE, COLOR_BLACK);
}

void set_header_text(char text[]) {
    char *header_new = malloc(sizeof(char) * COLS);
    int max = (COLS / 2) - (strlen(text) / 2), i = 0;

    /* fill spaces */
    for (; i < max; i++)
        header_new[i] = ' ';
    header_new[i] = '\0';
    strcat(header_new, text);
    i += strlen(text);
    for (; i < COLS; i++)
        header_new[i] = ' ';
    header_new[i] = '\0';

    attron(A_REVERSE);
    mvwaddstr(stdscr, 0, 0, header_new);
    attroff(A_REVERSE);

    refresh();
}

/* handles key events */
void mainloop() {
    int keyevent = -1;
    int y = 1, x = 5, scrollf = 0, reset_pos;

    while (keyevent != 'q' && keyevent != KEY_CANCEL) {
        keyevent = getch();

        switch (keyevent) {
            /* cursor movement */
            case KEY_UP:
                if (y > 1) {
                    y--;
                } else if (scrollf > 0) {
                    scroll_current(--scrollf);
                }
                move(LINES - 1, 0);
                clrtoeol();
                break;
            case KEY_DOWN:
                if (y < LINES - 2) {
                    y++;
                } else if ((unsigned)scrollf + (unsigned)LINES <= currentsite->meta->linecount) {
                    scroll_current(++scrollf);
                }
                move(LINES - 1, 0);
                clrtoeol();
                break;
            case KEY_LEFT:
                if (x > 5) {
                    x--;
                }
                move(LINES - 1, 0);
                clrtoeol();
                break;
            case KEY_RIGHT:
                if (x < COLS) {
                    x++;
                } else {
                    x = 5;
                    y++;
                }
                move(LINES - 1, 0);
                clrtoeol();
                break;
            /* site navigation */
            case 'o':
                prompt_url();
                x = 5;
                y = 1;
                scrollf = 0;
                break;
            case ' ':
            case KEY_ENTER:
            case 'f':
                reset_pos = 1;
                switch (currentsite->lines[y - 1 + scrollf]->ltype) {
                    case file:
                        currentsite = followplain(currentsite, y - 1 + scrollf);
                        break;
                    case directory:
                    case duplicate:
                        currentsite = followlink(currentsite, y - 1 + scrollf);
                        break;
                    case hyperlink:
                        /* telnet, cso and tn3270-telnet are their own protocols,
                           so they are opened in a browser/with xdg-open. */
                    case csoserver:
                    case telnetsession:
                    case tn3270session:
                        followhyper(currentsite, y - 1 + scrollf);
                        reset_pos = 0;
                        break;
                    case indexserver:
                        prompt_index(y - 1 + scrollf);
                        break;
                    case binhex:
                    case dosbinary:
                    case uuencoded:
                    case binaryfile:
                    case gifimage:
                    case image:
                    case document:
                    case soundfile:
                        prompt_download(y - 1 + scrollf);
                        reset_pos = 0;
                        break;
                    default:
                        reset_pos = 0;
                        break;
                }

                /* if link could be followed by nezumi itself, reset pos & add to history */
                if (reset_pos) {
                    if (currentsite) {
                        history[++histidx] = currentsite;
                        histmax = histidx;
                        scrollf = 0;
                        x = 5;
                        y = 1;
                        scroll_current(0);
                    } else {
                        currentsite = history[histidx];
                    }
                }
                break;
            case 'b':
            case 'p':
                hist_prev();
                scrollf = 0;
                x = 5;
                y = 1;
                move(LINES - 1, 0);
                clrtoeol();
                break;
            case 'n':
            case 'F':
                hist_next();
                scrollf = 0;
                x = 5;
                y = 1;
                move(LINES - 1, 0);
                clrtoeol();
                break;
        }

        move(y, x);
        refresh();
    }
}

/* hist_prev loads the previous page, if any, from cache */
void hist_prev() {
    if (histidx == 0) { /* safety check */
        return;
    }

    /* load page */
    currentsite = history[--histidx];

    /* display page */
    scroll_current(0);
}

/* hist_next loads the next page, if any, from cache */
void hist_next() {
    if (histidx == histmax) { /* safety check */
        return;
    }

    /* load page */
    currentsite = history[++histidx];

    /* display page */
    scroll_current(0);
}

/* prompts for an url entry and then loads it using load_page() */
void prompt_url() {
    char *prompt = malloc(sizeof(char) * COLS);
    int i = 9;

    /* fill spaces */
    strcpy(prompt, "open url:");
    for (; i < COLS; i++)
        prompt[i] = ' ';
    prompt[i] = '\0';

    attron(A_REVERSE);
    mvwaddstr(stdscr, LINES - 2, 0, prompt);
    move(LINES - 2, 10);

    free(prompt);

    refresh();

    char *gotourl = malloc(sizeof(char) * MAXURLLEN);
    echo();
    if (getnstr(gotourl, MAXURLLEN - 2) != ERR) {
        load_page(gotourl);
    } else {
        free(gotourl);
    }
    attroff(A_REVERSE);
    noecho();
}

/* prompts for an index search string and then requests
   the results from the currently selected index server */
void prompt_index(unsigned int linum) {
    char *prompt = malloc(sizeof(char) * COLS);
    int i = 13;

    /* fill spaces */
    strcpy(prompt, "search query:");
    for (; i < COLS; i++)
        prompt[i] = ' ';
    prompt[i] = '\0';

    attron(A_REVERSE);
    mvwaddstr(stdscr, LINES - 2, 0, prompt);
    move(LINES - 2, 14);

    free(prompt);

    refresh();

    char *squery = malloc(sizeof(char) * MAXURLLEN);
    echo();
    if (getnstr(squery, MAXURLLEN - 2) != ERR) {
        currentsite = followprompt(currentsite, linum, squery);
    } else {
        free(squery);
    }
    attroff(A_REVERSE);
    noecho();
}

/* prompts for a file name in the current directory and
   downloads the currently selected binary file */
void prompt_download(unsigned int linum) {
    char *prompt = malloc(sizeof(char) * COLS);
    int i = 8;

    /* fill spaces */
    strcpy(prompt, "save as:");
    for (; i < COLS; i++)
        prompt[i] = ' ';
    prompt[i] = '\0';

    attron(A_REVERSE);
    mvwaddstr(stdscr, LINES - 2, 0, prompt);
    move(LINES - 2, 9);

    free(prompt);

    refresh();

    char *fname = malloc(sizeof(char) * MAXURLLEN);
    echo();
    if (getnstr(fname, MAXURLLEN - 2) != ERR) {
        followbinary(currentsite, linum, fname);
    } else {
        free(fname);
    }
    attroff(A_REVERSE);
    noecho();
}

/* scrolls the current page n lines down */
void scroll_current(unsigned int factor) {
    unsigned int i;
    char linum[12];

    clear();

    set_header_text(currentsite->meta->url);

    for (i = 0; factor < currentsite->meta->linecount && i < (unsigned)LINES - 2; i++, factor++) {
        attron(COLOR_PAIR(2));
        sprintf(linum, "%4u", factor + 1);
        mvaddstr(i + 1, 0, linum);
        attroff(COLOR_PAIR(2));

        if (currentsite->lines[factor]->ltype == error) {
            attron(COLOR_PAIR(1));
        } else if (currentsite->lines[factor]->ltype != information) {
            attron(COLOR_PAIR(3));
        }

        mvaddstr(i + 1, 5, currentsite->lines[factor]->text);
        attroff(COLOR_PAIR(1));
        attroff(COLOR_PAIR(3));
    }
}

/* loads a page into the main area */
void load_page(char *url) {
    char *message = malloc(sizeof(char) * (strlen(url) + 12));
    strcpy(message, "loading ");
    strcat(message, url);
    strcat(message, "...");
    mvaddstr(LINES - 1, 0, message);
    refresh();

    struct simplepage *gsite = handleloadrequest(url);
    char linum[12];
    unsigned int i;

    if (gsite == NULL) {
        return;
    }

    history[++histidx] = gsite;
    currentsite = gsite;
    histmax = histidx;

    clear();

    set_header_text(gsite->meta->url);

    for (i = 0; i < (unsigned)LINES - 2 && i < gsite->meta->linecount; i++) {
        attron(COLOR_PAIR(2));
        sprintf(linum, "%4u", i + 1);
        mvaddstr(i + 1, 0, linum);
        attroff(COLOR_PAIR(2));

        if (gsite->lines[i]->ltype == error) {
            attron(COLOR_PAIR(1));
        } else if (gsite->lines[i]->ltype != information) {
            attron(COLOR_PAIR(3));
        }

        mvaddstr(i + 1, 5, gsite->lines[i]->text);
        attroff(COLOR_PAIR(1));
        attroff(COLOR_PAIR(3));
    }

    strcpy(message, "loaded ");
    strcat(message, gsite->meta->url);
    mvaddstr(LINES - 1, 0, message);

    move(1, 5);

    refresh();

    free(message);
}
