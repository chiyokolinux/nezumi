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
void bookmark_add_prompt();
void load_bookmarks();
void add_bookmark(struct bookmark *bm, int disable_mem_write);
int endswith(const char *str, const char *suffix);

struct simplepage *currentsite = NULL;
struct simplepage **history = NULL;
struct bookmark_list *bookmarks = NULL;
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

    /* calloc history */
    history = calloc(HISTSIZE, sizeof(struct simplepage *));

    set_header_text("welcome to nezumi!");

    load_bookmarks();

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
    int y = 1, x = 5, scrollf = 0, reset_pos, move_cursor;

    while (keyevent != 'q' && keyevent != KEY_CANCEL) {
        keyevent = getch();

        move_cursor = 1;
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
                        move_cursor = 0;
                        break;
                    case bookmark:
                        load_page(currentsite->lines[y - 1 + scrollf]->host);
                        x = 5;
                        y = 1;
                        reset_pos = 0;
                        break;
                    default:
                        reset_pos = 0;
                        break;
                }

                /* if link could be followed by nezumi itself, reset pos & add to history */
                if (reset_pos) {
                    if (currentsite) {
                        if (history[++histidx % HISTSIZE]) {
                            freesimplepage(history[histidx % HISTSIZE], 0);
                        }
                        history[histidx % HISTSIZE] = currentsite;
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
            case 'r':
                load_page(currentsite->meta->url);
                scroll_current(scrollf);
                break;
            case 'd': /* most browsers today use ctrl+d for bookmarking */
                bookmark_add_prompt();
                break;
            case 'D':
                currentsite = genbookmarkspage(bookmarks);
                if (history[++histidx % HISTSIZE]) {
                    freesimplepage(history[histidx % HISTSIZE], 0);
                }
                history[histidx % HISTSIZE] = currentsite;
                histmax = histidx;
                scrollf = 0;
                x = 5;
                y = 1;
                scroll_current(0);
                break;
        }

        if (move_cursor) {
            move(y, x);
        }
        refresh();
    }
}

/* hist_prev loads the previous page, if any, from cache */
void hist_prev() {
    if (histidx == 0) { /* safety check */
        return;
    }

    /* load page */
    currentsite = history[--histidx % HISTSIZE];

    /* display page */
    scroll_current(0);
}

/* hist_next loads the next page, if any, from cache */
void hist_next() {
    if (histidx == histmax) { /* safety check */
        return;
    }

    /* load page */
    currentsite = history[++histidx % HISTSIZE];

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

    move(LINES - 1, 0);
    clrtoeol();

    attron(A_REVERSE);
    mvwaddstr(stdscr, LINES - 2, 0, prompt);
    move(LINES - 2, 10);

    free(prompt);

    refresh();

    char *gotourl = malloc(sizeof(char) * MAXURLLEN);
    echo();
    if (getnstr(gotourl, MAXURLLEN - 2) != ERR) {
        attroff(A_REVERSE);
        if (!endswith(gotourl, "\a\a")) {
            load_page(gotourl);
        } else {
            attroff(A_REVERSE);
            free(gotourl);
        }
    } else {
        attroff(A_REVERSE);
        free(gotourl);
    }
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

    move(LINES - 1, 0);
    clrtoeol();

    attron(A_REVERSE);
    mvwaddstr(stdscr, LINES - 2, 0, prompt);
    move(LINES - 2, 14);

    free(prompt);

    refresh();

    char *squery = malloc(sizeof(char) * MAXURLLEN);
    echo();
    if (getnstr(squery, MAXURLLEN - 2) != ERR) {
        attroff(A_REVERSE);
        if (!endswith(squery, "\a\a")) {
            currentsite = followprompt(currentsite, linum, squery);
        } else {
            attroff(A_REVERSE);
            free(squery);
        }
    } else {
        attroff(A_REVERSE);
        free(squery);
    }
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

    move(LINES - 1, 0);
    clrtoeol();

    attron(A_REVERSE);
    mvwaddstr(stdscr, LINES - 2, 0, prompt);
    move(LINES - 2, 9);

    free(prompt);

    refresh();

    char *fname = malloc(sizeof(char) * MAXURLLEN);
    echo();
    if (getnstr(fname, MAXURLLEN - 2) != ERR) {
        attroff(A_REVERSE);
        if (!endswith(fname, "\a\a")) {
            followbinary(currentsite, linum, fname);
        } else {
            free(fname);
        }
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

    if (history[++histidx % HISTSIZE]) {
        freesimplepage(history[histidx % HISTSIZE], 0);
    }
    history[histidx % HISTSIZE] = gsite;
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

void bookmark_add_prompt() {
    char *prompt = malloc(sizeof(char) * COLS);
    int i = 14;

    /* fill spaces */
    strcpy(prompt, "bookmark name:");
    for (; i < COLS; i++)
        prompt[i] = ' ';
    prompt[i] = '\0';

    move(LINES - 1, 0);
    clrtoeol();

    attron(A_REVERSE);
    mvwaddstr(stdscr, LINES - 2, 0, prompt);
    move(LINES - 2, 15);

    free(prompt);

    refresh();

    char *bmname = malloc(sizeof(char) * 255);
    echo();
    if (getnstr(bmname, 254) != ERR) {
        attroff(A_REVERSE);
        if (!endswith(bmname, "\a\a")) {
            struct bookmark *bm = malloc(sizeof(struct bookmark));
            strncpy(bm->name, bmname, 255);
            strncpy(bm->url, currentsite->meta->url, 1022);
            bm->url[1022] = '\0'; /* ensure null termination */
            add_bookmark(bm, 0);
        }
    }
    free(bmname);
    attroff(A_REVERSE);
    move(LINES - 2, 0);
    clrtoeol();
    noecho();
}

/* read bookmarks file into bookmark list */
void load_bookmarks() {
    FILE *bmFile;
    char path[512];
    strcat(strcpy(path, getenv("HOME")), "/.config/nezumi-bookmarks");
    bmFile = fopen(path, "r");

    if (!bmFile) {
        mvaddstr(LINES - 1, 0, "bookmarks file does not exist, creating...");

        /* create first entry
           calloc because we do not want uninitialized
           memory written to our bookmarks file        */
        struct bookmark *first_entry = calloc(1, sizeof(struct bookmark));
        strcpy(first_entry->name, "Floodgap Gopher");
        strcpy(first_entry->url, "gopher://gopher.floodgap.com/");
        add_bookmark(first_entry, 1);

        /* alloc bookmark list struct */
        bookmarks = malloc(sizeof(struct bookmark_list));
        bookmarks->length = 1;

        /* alloc actual list & add first entry */
        bookmarks->bookmarks = malloc(sizeof(struct bookmark *) * 32);
        bookmarks->alloc_length = 32;
        bookmarks->bookmarks[0] = first_entry;
    } else {
        /* alloc bookmark list struct */
        bookmarks = malloc(sizeof(struct bookmark_list));
        bookmarks->bookmarks = malloc(sizeof(struct bookmark *) * 32);
        bookmarks->alloc_length = 32;
        bookmarks->length = 0;

        char buf[1282];

        while (fgets(buf, 1281, bmFile) != NULL) {
            /* alloc entry & write data */
            struct bookmark *entry = malloc(sizeof(struct bookmark));
            strncpy(entry->name, buf, 255);
            strncpy(entry->url, buf + 256, 1023);
            bookmarks->bookmarks[bookmarks->length] = entry;

            bookmarks->length++;
            if (bookmarks->length >= bookmarks->alloc_length) {
                bookmarks->alloc_length += 32;
                bookmarks->bookmarks = realloc(bookmarks->bookmarks, sizeof(struct bookmark *) * bookmarks->alloc_length);
            }
        }
    }
}

/* add a bookmark and save it to disk */
void add_bookmark(struct bookmark *bm, int disable_mem_write) {
    FILE *bmFile;
    char path[512];
    strcat(strcpy(path, getenv("HOME")), "/.config/nezumi-bookmarks");
    bmFile = fopen(path, "a");

    if (!bmFile) {
        mvaddstr(LINES - 1, 0, "error: fopen: ");
        addstr(strerror(errno));
    } else {
        char tmp[3] = "\0\n";

        fwrite(bm->name, 255, 1, bmFile);
        fwrite(tmp, 1, 1, bmFile);
        fwrite(bm->url, 1023, 1, bmFile);
        fwrite(tmp, 1, 1, bmFile);

        fclose(bmFile);
    }

    if (!disable_mem_write) {
        bookmarks->bookmarks[bookmarks->length] = bm;

        bookmarks->length++;
        if (bookmarks->length >= bookmarks->alloc_length) {
            bookmarks->alloc_length += 32;
            bookmarks->bookmarks = realloc(bookmarks->bookmarks, sizeof(struct bookmark *) * bookmarks->alloc_length);
        }
    }
}

/* string ends-with helper function */
int endswith(const char *str, const char *suffix) {
    if (!str || !suffix)
        return 0;
    int lenstr = strlen(str);
    int lensuffix = strlen(suffix);
    if (lensuffix > lenstr)
        return 0;
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}
