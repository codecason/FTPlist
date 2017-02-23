/*
File: spider.c
Project: FTP Crawler
Author: Peter Salanki (peter@trops.net ICQ: 54915721)
License: GPL
*/

#include <ftpcrawler.h>
int verbosity= VERBOSITY_NORMAL;

// First, let's make a prototype for the worm.
int worm(char dir[], fxp_handle_t fxp, MYSQL *sock, char ftp[FTPSIZE]);

void hatchspider(char ftp[FTPSIZE])
{
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

  sprintf(query, "DELETE FROM files WHERE ftp = '%i'", atoi(ftp));

  if(mysql_query(sock, query))
    {
      printf("Delete FAILED (%s)\n",mysql_error(sock));
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
  }
  */

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

  /*
    Now, LOCK the ftp so no other procs can take it and set RED
  */
  strcpy(query, "");
  strcat(query, "UPDATE ftp SET status = '2', locked = '1' WHERE id = '");
  strcat(query, ftp);
  strcat(query, "'");
  mysql_query(sock, query);

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
  // Let's start rocking!
  worm("/", fxp, sock, ftp);

  mess("" CBLUE "Spider for FTP: %s ended." COFF "\n", ftp);

  strcpy(query, "");
  strcat(query, "UPDATE ftp SET locked = '0' WHERE id = '");
  strcat(query, ftp);
  strcat(query, "'");
  mysql_query(sock, query);

  // Well, now just close the sockets
  fxp_disconnect(fxp);
  fxp_destroy(&fxp);
  mysql_close(sock);


 die:
  strcpy(query, "");
  strcat(query, "UPDATE ftp SET locked = '0' WHERE id = '");
  strcat(query, ftp);
  strcat(query, "'");
  mysql_query(sock, query);
  fxp_disconnect(fxp);
  fxp_destroy(&fxp);
  mysql_close(sock);

}

/* The worm */

int worm(char dir[], fxp_handle_t fxp, MYSQL *sock, char ftp[FTPSIZE]) {
  list_t *files= list_create();
  int i;
  // unsigned long long size;
  char wormdir[1024];
  char parsed_dir[1024];
  char parsed_file[1024];
  char sql[1024];
#ifdef DEBUG
  mess("" CGREEN "WORM SPAWANED FOR DIR: %s" COFF "\n", dir);
#endif


  if(fxp_list(fxp, dir, files) != FXPOK) {
    printf("%s: fxp_list() failed: %s\n", ftp, dir);

  }

  for(i=0; i<list_count(files); i++) {

    fxp_complete_file_t *ptr= list_get(files, i);
    if (ptr->type == FXP_FILE_TYPE_DIRECTORY && strcmp(ptr->name, "./") != 0 && strcmp(ptr->name, ".") != 0 && strcmp(ptr->name, "../") != 0 && strcmp(ptr->name, "..") != 0) {
#ifdef DEBUG
      mess("[dir]  " CBLUE "%s" COFF "\n", ptr->name);
#endif
      strcpy(wormdir, "");
      strcat(wormdir, dir);
      if (strcmp (dir, "/") != 0)
	strcat(wormdir, "/");
      strcat(wormdir, ptr->name);

      // Fix the query
      strcpy(parsed_dir, "");
      strcpy(parsed_file, "");
      sqlfix(dir, parsed_dir);
      sqlfix(ptr->name, parsed_file);

      sprintf(sql, "INSERT INTO files (ftp, filename, path, type, size) VALUES ('%s', '%s', '%s', '%s', '%d')", ftp,  parsed_file, parsed_dir, "dir", ptr->file_size);

      if(mysql_query(sock, sql))
	{
	  printf("%s: ", ftp);
	  printf("Query failed (%s)\n",mysql_error(sock));

	}
      worm(wormdir, fxp, sock, ftp);
    }

    else if (ptr->type == FXP_FILE_TYPE_REGULAR) {
#ifdef DEBUG
      mess("[file] " CRED "%s" COFF "\n", ptr->name);
#endif

      // Fix the query
      strcpy(parsed_dir, "");
      strcpy(parsed_file, "");
      sqlfix(dir, parsed_dir);
      sqlfix(ptr->name, parsed_file);


      sprintf(sql, "INSERT INTO files (ftp, filename, path, type, size) VALUES ('%s', '%s', '%s', '%s', '%d')", ftp, parsed_file, parsed_dir, "file", ptr->file_size);
      if(mysql_query(sock, sql))
	{
	  printf("%s: ", ftp);
	  printf("Query failed SQL: %s (%s)\n", sql, mysql_error(sock));

	}

    }

  }

  list_destroy(files);

  return 0;
}
