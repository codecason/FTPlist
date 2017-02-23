/*
  File: mother.c
  Project: FTP Crawler
  Author: Peter Salanki (peter@trops.net ICQ: 54915721)
  License: GPL
*/
#include <ftpcrawler.h>

void real_main(void)
{
  MYSQL mysql, *sock;
  MYSQL_RES *res;
  MYSQL_ROW row; 
  char query[512];

  mysql_init(&mysql);
  if (!(sock = mysql_real_connect(&mysql,MYSQL_SERVER,MYSQL_USR,MYSQL_PASS,MYSQL_DB,3306,NULL,0)))
    {
      fprintf(stderr,"Couldn't connect to MySQL server!\n%s\n\n",mysql_error(&mysql));
      perror("");

      exit(1);
    }

  sprintf(query, "UPDATE `status` SET `lastcrawl` = '%i'", time(NULL));
  mysql_query(sock, query);

  mysql_query(sock, "SELECT * FROM ftp WHERE locked = '0'");
  res = mysql_store_result(sock);   

  while ((row = mysql_fetch_row(res))) {
    if (!fork()) { 
      hatchspider(row[0]);
      exit(0); 
    }
  }

  mysql_close(sock);

}
