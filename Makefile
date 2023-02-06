# SPDX-License-Identifier: LGPL-2.1-or-later
-include config.mk
CFLAGS?=-W -Wall -O2
CFLAGS+=-std=gnu99
PREFIX?=/usr/local/
LIBDIR?=$(PREFIX)/lib
BINDIR?=$(PREFIX)/bin
INCLUDEDIR?=$(PREFIX)/include

include proj.mk

LIB_OBJS=$(subst .c,.o,$(LIB_SRCS))
LIB_DEPS=$(subst .c,.dep,$(LIB_SRCS))
LIB_SNAME=lib$(LIB).a
LIB_NAME=lib$(LIB).so
LIB_SONAME=$(LIB_NAME).1
LIB_FILE=$(LIB_SONAME).0.0

BIN_OBJS=$(subst .c,.o,$(BIN_SRCS))
BIN_DEPS=$(subst .c,.dep,$(BIN_SRCS))

all: $(BIN) $(LIB_FILE) $(LIB_SONAME) $(LIB_NAME) $(LIB_SNAME)

$(LIB_OBJS): CFLAGS+=-fPIC
$(LIB_FILE): LDLIBS+=$(LIB_LDLIBS)

#ifdef BIN
$(BIN): $(BIN_OBJS)
$(BIN): LDLIBS+=-l$(LIB)
$(BIN): LDFLAGS+=-L.
$(BIN): |$(LIB_NAME) $(LIB_SONAME)
#endif

LIB_FILES=$(LIB_FILE) $(SLIB_NAME) $(LIB_NAME) $(LIB_SONAME)

-include $(LIB_DEPS)
-include $(BIN_DEPS)

$(LIB_FILE): $(LIB_OBJS)
	 $(CC) -fPIC --shared -Wl,-soname -Wl,$(LIB_SONAME) $^ $(LIB_LDLIBS) -o $@

$(LIB_SONAME): $(LIB_FILE)
	[ -f $(LIB_SONAME) ] || ln -s $(LIB_FILE) $(LIB_SONAME)

$(LIB_NAME): $(LIB_FILE)
	[ -f $(LIB_NAME) ] || ln -s $(LIB_FILE) $(LIB_NAME)

$(LIB_SNAME): $(LIB_OBJS)
	$(AR) rcs $@ $^

$(BIN_DEPS) $(LIB_DEPS): %.dep: %.c
	$(CC) -MM $(CFLAGS) $< -o $@

install:
	install -d $(DESTDIR)/$(LIBDIR)
	install $(LIB_FILE) $(LIB_SNAME) -t $(DESTDIR)/$(LIBDIR)
	cp -d $(LIB_NAME) $(LIB_SONAME) $(DESTDIR)/$(LIBDIR)
	install -d $(DESTDIR)/$(INCLUDEDIR)
	install $(LIB_HEADERS) -t $(DESTDIR)/$(INCLUDEDIR)
	[ -z "$(BIN)" ] || install -d $(DESTDIR)/$(BINDIR)
	[ -z "$(BIN)" ] || install $(BIN) -t $(DESTDIR)/$(BINDIR)

clean:
	rm -f $(LIB_NAME) $(LIB_SONAME) $(LIB_FILE) $(LIB_SNAME) $(LIB_OBJS) $(BIN) $(BIN_OBJS) $(LIB_DEPS) $(BIN_DEPS) config.mk
