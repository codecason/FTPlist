
CC=gcc
CFLAGS=-Wall -I. -Ilibmillweed
PREFIX=/usr/local

ifeq "$(DEBUG)" "1"
CFLAGS:=-ggdb $(CFLAGS)
endif

libfxp_SEGS=fxp_main.o fxp_state.o fxp_control.o fxp_util.o fxp_data.o fxp_file_ops.o fxp_nav_ops.o \
	fxp_list_ops.o fxp_transfer_ops.o

all : libfxp.a

libmillweed/libmillweed.a :
	cd libmillweed && $(MAKE)

libfxp.a : libmillweed/libmillweed.a $(libfxp_SEGS)
	ld -r -o libfxp.o -u parse_url $(libfxp_SEGS) -Llibmillweed -lmillweed
	ar -rcs $@ libfxp.o

install : libfxp.a
	mkdir -p $(PREFIX)/lib
	mkdir -p $(PREFIX)/include
	cp libfxp.a $(PREFIX)/lib
	cp fxp.h $(PREFIX)/include
	cd libmillweed && $(MAKE) install_headers

uninstall :
	rm -f $(PREFIX)/lib/libfxp.a
	rm -f $(PREFIX)/include/fxp.h
	cd libmillweed && $(MAKE) uninstall_headers

clean :
	rm -f \#* *~ .*~ core libfxp.a libfxp.o $(libfxp_SEGS)
	cd libmillweed && $(MAKE) clean
