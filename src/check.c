/*
  File: check.c
  Project: FTP Crawler
  Author: Peter Salanki (peter@trops.net ICQ: 54915721)
  License: GPL
*/

#include <ftpcrawler.h>

void check(char ftp[FTPSIZE]) {
  url_t url;
  fxp_handle_t fxp;
  MYSQL mysql, *sock;
  MYSQL_RES *res;
  MYSQL_ROW row;
  char query[512]="";
  char real_url[1024]= "ftp://";
   
  mysql_init(&mysql);

  if (!(sock = mysql_real_connect(&mysql,MYSQL_SERVER,MYSQL_USR,MYSQL_PASS,MYSQL_DB,3306,NULL,0)))
    {
      printf("%s: MySQL conn failed!!", ftp);
      perror("");

      exit(1);
    }
  strcpy(query, "");
  strcat(query, "SELECT * FROM ftp WHERE id = '");
  strcat(query, ftp);
  strcat(query, "'");
  mysql_query(sock, query);
  res = mysql_store_result(sock);   
  row = mysql_fetch_row(res);
 /* if (strcmp(row[7], "1") == 0) {
    printf("!! %s: Allready locked !!\n", ftp);
    exit(1);
  } */
  strcat(real_url, row[1]);
  strcat(real_url, ":");
  strcat(real_url, row[2]);

  if(parse_url(real_url, &url) != 0) {
    printf("%s: parse_url() failed\n", ftp);
    error("parse_url() failed\n");

  }
  if(*url.path == 0x0)
    strncpy(url.path, "/", sizeof(url.path));
  if(fxp_init(&fxp, &url) != FXPOK) {
    printf("%s: fxp_init() failed\n", ftp);
    error("fxp_init() failed\n");
  }

  strcpy(query, "");
  strcat(query, "UPDATE ftp SET status = '2' WHERE id = '");
  strcat(query, ftp);
  strcat(query, "'");
  mysql_query(sock, query);

  /*
    Now, LOCK the ftp so no other procs can take it and set RED
  */

  if(fxp_connect(fxp, 10000) != FXPOK) {
    printf("%s: fxp_connect() failed\n", ftp);
    error("fxp_connect() failed\n");
    goto die;
  }
  
strcpy(query, "");
  strcat(query, "UPDATE ftp SET status = '1' WHERE id = '");
  strcat(query, ftp);
  strcat(query, "'");
  mysql_query(sock, query);

  if(fxp_login(fxp, row[3], row[4]) != FXPOK) {
    printf("%s: fxp_login() failed\n", ftp);
    error("fxp_login() failed\n");
    goto die;
  }

  strcpy(query, "");
  strcat(query, "UPDATE ftp SET status = '0' WHERE id = '");
  strcat(query, ftp);
  strcat(query, "'");
  mysql_query(sock, query);

  mess("" CGREEN "Check for FTP: %s ended." COFF "\n", ftp);

  // Well, now just close the sockets
  fxp_disconnect(fxp);
  fxp_destroy(&fxp);
  // mysql_close(sock);


 die:
  fxp_disconnect(fxp);
  fxp_destroy(&fxp);
  //  mysql_close(sock);
}
