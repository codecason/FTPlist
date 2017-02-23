#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <mysql/mysql.h>
#include <unistd.h>

#include <fxp.h>

#define VERSION "0.20"
#define FTPSIZE 20
#define MYSQL_SERVER "127.0.0.1"
#define MYSQL_DB "ftplist"
#define MYSQL_USR "mypass"
#define MYSQL_PASS "thomas"

/*
 Status levels
 0 = Green
 1 = Yellow
 2 = Red
*/

void real_main(void);
void mother_check(void);   
void check(char ftp[20]);
void hatchspider(char ftp[FTPSIZE]);
void sqlfix(char in[], char out[]);

