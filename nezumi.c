#include <stdio.h>
#include <string.h>

#include <curses.h>

#include "networking.h"
#include "parser.h"

void init_colors();
void set_header_text(char text[]);
void load_page(char *url);

int main(void) {
    /* init curses */
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, true);
    wclear(stdscr);
    start_color();
    init_colors();

    set_header_text("welcome to nezumi!");

    load_page(strdup("gopher://gopher.floodgap.com/"));
    sleep(10);
    
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
    init_pair(3, COLOR_BLACK, COLOR_WHITE);
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

/* loads a page into the main area */
void load_page(char *url) {
    struct simplepage *gsite = handleloadrequest(url);

    set_header_text(gsite->meta->url);

    for (unsigned int i = 0; i < gsite->meta->linecount; i++) {
        if (gsite->lines[i]->ltype == error) {
            attron(COLOR_PAIR(1));
        } else if (gsite->lines[i]->ltype != information) {
            attron(COLOR_PAIR(2));
        }
        mvwaddstr(stdscr, i + 1, 0, gsite->lines[i]->text);
        attroff(COLOR_PAIR(1));
        attroff(COLOR_PAIR(2));
    }

    refresh();
}
