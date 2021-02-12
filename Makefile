# Makefile for nezumi
# See LICENSE file for license and copyright information.

include config.mk

PROGBIN = $(NAME)

PROGOBJ = nezumi.o
NETWOBJ = networking.o
PARSOBJ = parser.o

OBJECTS = $(NETWOBJ) $(PARSOBJ) $(PROGOBJ)
HEADERS = config.h networking.h parser.h

all: $(PROGBIN)

$(PROGBIN): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS)

$(PROGOBJ): $(HEADERS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c -o $@ $<

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f $(PROGBIN) $(DESTDIR)$(PREFIX)/bin
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	sed "s/1.0/$(VERSION)/g" < $(NAME).1 > $(DESTDIR)$(MANPREFIX)/man1/$(NAME).1

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(PROGBIN)
	rm -f $(DESTDIR)$(MANPREFIX)/man1/$(NAME).1

dist: clean
	mkdir -p $(NAME)-$(VERSION)
	cp -f Makefile README config.mk *.c *.h nezumi.1 $(NAME)-$(VERSION)
	tar -cf $(NAME)-$(VERSION).tar $(NAME)-$(VERSION)
	gzip $(NAME)-$(VERSION).tar
	rm -rf $(NAME)-$(VERSION)

clean:
	rm -f $(PROGBIN) *.o $(NAME)-$(VERSION).tar.gz

.SUFFIXES: .def.h

.def.h.h:
	cp $< $@

.PHONY:
	all install uninstall dist clean
