-include include.mk
CFLAGS?=-W -Wall -O2
PREFIX?=/usr/local/
LIBDIR?=$(PREFIX)/lib
INCLUDEDIR?=$(PREFIX)/include

LDLIBS=-lz

LIB=libstardict
SLIB_NAME=$(LIB).a
LIB_NAME=$(LIB).so
LIB_SONAME=$(LIB_NAME).1
LIB_FILE=$(LIB_SONAME).0.0
LIB_OBJS=libstardict.o

libstardict.o: CFLAGS=-fPIC

LIB_FILES=$(LIB_FILE) $(SLIB_NAME) $(LIB_NAME) $(LIB_SONAME)

all: sd-cmd $(LIB_FILES)

sd-cmd: sd-cmd.o libstardict.o

$(LIB_FILE): $(LIB_OBJS)
	 $(CC) -fPIC --shared -Wl,-soname -Wl,$(LIB_SONAME) $^ $(LDLIBS) -o $@

$(LIB_SONAME): $(LIB_FILE)
	ln -s $(LIB_FILE) $(LIB_SONAME)

$(LIB_NAME): $(LIB_FILE)
	ln -s $(LIB_FILE) $(LIB_NAME)

$(SLIB_NAME): $(LIB_OBJS)
	$(AR) rcs $@ $^

install:
	install -D $(LIB_FILE) -t $(DESTDIR)/$(LIBDIR)
	cp -d $(LIB_NAME) $(LIB_SONAME) $(DESTDIR)/$(LIBDIR)
	install -D libstardict.h -t $(DESTDIR)/$(INCLUDEDIR)

clean:
	rm -f *.o sd-cmd $(LIB_NAME) $(LIB_SONAME) $(LIB_FILE)
