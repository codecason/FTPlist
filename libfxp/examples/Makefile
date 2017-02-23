
CC=gcc
CFLAGS=-Wall -ggdb -D_FILE_OFFSET_BITS=64 -I.. -I../libmillweed
LIBS=../libfxp.a
TARGETS=fxpget fxpnlst

all : $(TARGETS)

$(TARGETS) : $(LIBS)

clean :
	rm -f \#* *~ .*~ core *.o $(TARGETS)
