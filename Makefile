# Makefile for the FTP Crawler

prefix=/usr/local
bindir=$(prefix)/bin

CC=gcc
CFLAGS=-g -O2 -Wall -Isrc/ -I/usr/local/include/ -Ilibmillweed
LDLIBS=-L/usr/local/lib/mysql -lmysqlclient

TARGET=ftpcrawler

SRC = src/check.c src/main.c src/mother.c src/mother_check.c src/spider.c src/sqlfix.c
OBJS = src/check.o src/main.o src/mother.o src/mother_check.o src/spider.o src/sqlfix.o libfxp/libfxp.a

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDLIBS)

clean:
	rm -f src/*.o src/*~ ftpcrawler
