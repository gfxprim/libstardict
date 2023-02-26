-include ../config.mk
SOURCE_PAGES=$(wildcard *.sdc)
MAN_PAGES=$(subst .sdc,,$(SOURCE_PAGES))
INSTALL_PAGES=$(wildcard *.3)
PREFIX?=/usr/local/
MANDIR?=$(PREFIX)/share/man

all: $(MAN_PAGES)

install: $(INSTALL_PAGES)
	install -d $(DESTDIR)/$(MANDIR)/man3/
	cp -d $(INSTALL_PAGES) -t $(DESTDIR)/$(MANDIR)/man3/

$(MAN_PAGES): %: %.sdc
	scdoc < $< > $@

clean:
	rm -f $(MAN_PAGES)
