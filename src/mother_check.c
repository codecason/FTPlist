/*
  File: mother_checker.c
  Project: FTP Crawler
  Author: Peter Salanki (peter@trops.net ICQ: 54915721)
  License: GPL
*/

#include <ftpcrawler.h>

void mother_check(void)
{
  MYSQL mysql, *sock;
  MYSQL_RES *res;
  MYSQL_ROW row; 
  mysql_init(&mysql);
  if (!(sock = mysql_real_connect(&mysql,MYSQL_SERVER,MYSQL_USR,MYSQL_PASS,MYSQL_DB,3306,NULL,0)))
    {
      fprintf(stderr,"Couldn't connect to MySQL server!\n%s\n\n",mysql_error(&mysql));
      perror("");

      exit(1);
    }


  while(1) {
    printf("\nNew check\n");
    mysql_query(sock, "SELECT id FROM ftp");
    res = mysql_store_result(sock);   

    while ((row = mysql_fetch_row(res))) {

      if (!fork()) { 
	check(row[0]); 
	exit(0); 
      }

    }
    sleep(400);
  }
  mysql_close(sock);

}
