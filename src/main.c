/*
  File: main.c
  Project: FTP Crawler
  Author: Peter Salanki (peter@trops.net ICQ: 54915721)
  License: GPL
*/

#include <ftpcrawler.h>
#define ARGTXT "FTP Crawler\t%s\n\nValid argumets are:\nmother\t\tMain crawler process\nmother_check\tMain checking feature\nunlock\t\tUnlock all ftps.\n"

int main(int argc, char *argv[])
{
  MYSQL mysql, *sock;

  if (argc == 1)  printf(ARGTXT, VERSION);

  else if (strcmp(argv[1], "mother") == 0) {
    printf("FTP Crawler\n%s\n", VERSION);
#ifdef DEBUG
    printf("DEBUG MODE\n");
    printf("Starting main process\n");
#endif
    real_main();
  }

  else if (strcmp(argv[1], "mother_check") == 0) {
    printf("FTP Crawler check\n%s\n", VERSION);
#ifdef DEBUG
    printf("DEBUG MODE\n");
    printf("Starting check process\n");
#endif
    mother_check();
  }

  else if (strcmp(argv[1], "unlock") == 0) {
    printf("FTP Crawler check\n%s\n", VERSION);
#ifdef DEBUG
    printf("DEBUG MODE\n");
    printf("Setting locked = 0 on all.\n");
#endif
    mysql_init(&mysql);

    if (!(sock = mysql_real_connect(&mysql,MYSQL_SERVER,MYSQL_USR,MYSQL_PASS,MYSQL_DB,3306,NULL,0)))
      {
	fprintf(stderr,"Couldn't connect to MySQL server!\n%s\n\n",mysql_error(&mysql));
	perror("");

	exit(1);
      }

    mysql_query(sock, "UPDATE `ftp` SET `locked` = '0'");
  }

  else printf(ARGTXT, VERSION);
  return 0;
}
