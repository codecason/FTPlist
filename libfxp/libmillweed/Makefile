
CC=gcc
CFLAGS=-Wall -I.
PREFIX=/usr/local

ifeq "$(DEBUG)" "1"
CFLAGS:=-ggdb $(CFLAGS)
endif

all : libmillweed.a

libmillweed_SEGS=list.o parse.o net.o

libmillweed.a : $(libmillweed_SEGS)
	ar -rcs $@ $(libmillweed_SEGS)

install_headers :
	mkdir -p $(PREFIX)/include
	mkdir -p $(PREFIX)/include/millweed
	cp millweed/*.h $(PREFIX)/include/millweed

uninstall_headers :
	rm -fr $(PREFIX)/include/millweed

install : install_headers libmillweed.a
	mkdir -p $(PREFIX)/lib
	cp libmillweed.a $(PREFIX)/lib

uninstall : uninstall_headers
	rm -f $(PREFIX)/lib/libmillweed.a

clean :
	rm -f \#* *~ .*~ core libmillweed.a $(libmillweed_SEGS)
