
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <regex.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>

//ftpsearch and millweed headers
#include <millweed/misc.h>
#include <millweed/list.h>
#include <millweed/parse.h>

#define legal "[^\";/\\?:@&=\\+\\$,]"

int url_to_string(const url_t *url, char *out_buff, unsigned out_buff_size)
{
  char buff[256];

  if(url == NULL || out_buff == NULL)
    return -1;
  if(out_buff_size < 1) {
    *out_buff= 0x0;
    return 0;
  }
  if(*url->proto == 0x0 || *url->host == 0x0)
    return -1;

  snprintf(out_buff, out_buff_size, "%s://", url->proto);
  if(*url->user != 0x0 && strncmp(url->user, URL_DEFAULT_USERNAME, sizeof(url->user)) != 0) {
    strncat(out_buff, url->user, out_buff_size-strlen(out_buff));
    if(*url->pass != 0x0 && strncmp(url->pass, URL_DEFAULT_PASSWORD, sizeof(url->pass)) != 0) {
      snprintf(buff, sizeof(buff), ":%s", url->pass);
      strncat(out_buff, buff, out_buff_size-strlen(out_buff));
    }
    strncat(out_buff, "@", out_buff_size-strlen(out_buff));
  }
  strncat(out_buff, url->host, out_buff_size-strlen(out_buff));
  if(url->port > 0 && url->port != 21) {
    snprintf(buff, sizeof(buff), ":%d", url->port);
    strncat(out_buff, buff, out_buff_size-strlen(out_buff));
  }
  if(*url->path != 0x0)
    strncat(out_buff, url->path, out_buff_size-strlen(out_buff));

  return 0;
}

int parse_url(const char *url_string, url_t *out_url)
{
  static int compiled= 0;
  static regex_t proto_re, userpass_re, hostport_re;

  regmatch_t match[1];
  char *urlcpy, *tmp, *ptr, *tok, buff[BUFFSZ];

  if(!compiled) {
    if(regcomp(&proto_re, legal"+://", REG_EXTENDED | REG_ICASE) != 0) {
      error2("regcomp(proto_re) returned nonzero, returning [%s]\n", strerror(errno));
      return -1;
    }
    if(regcomp(&userpass_re, legal"+(:"legal"+)?@", REG_EXTENDED | REG_ICASE) != 0) {
      error2("regcomp(userpass_re) returned nonzero, returning [%s]\n", strerror(errno));
      return -1;
    }
    if(regcomp(&hostport_re, legal"+(:[0-9]+)?", REG_EXTENDED | REG_ICASE) != 0) {
      error2("regcomp(hostport_re) returned nonzero, returning [%s]\n", strerror(errno));
      return -1;
    }
    compiled= 1;
  }

  memset(out_url, 0, sizeof(url_t));
  tmp= urlcpy= strdup(url_string);

  //proto
  if(regexec(&proto_re, tmp, 1, match, 0) == 0) {
    if(match[0].rm_so != 0)
      error2("SHIT!  match[0].rm_so != 0\n");
    strncpy(out_url->proto, strtok_mw(tmp, ":", &ptr), sizeof(out_url->proto));
    tmp += match[0].rm_eo - match[0].rm_so;
  }
  else {
    error2("error parsing out proto, returning\n");
    goto parse_url_die;
  }

  //optional username, optional password
  if(regexec(&userpass_re, tmp, 1, match, 0) == 0) {
    if(match[0].rm_so != 0)
      error2("SHIT!  match[0].rm_so != 0\n");
    strncpy(buff, strtok_mw(tmp, "@", &ptr), sizeof(buff));
    strncpy(out_url->user, strtok_mw(buff, ":", &ptr), sizeof(out_url->user));
    if((tok= strtok_mw(NULL, "", &ptr)) != NULL)
      strncpy(out_url->pass, (tok == NULL)? URL_DEFAULT_PASSWORD: tok, sizeof(out_url->pass));
    tmp += match[0].rm_eo - match[0].rm_so;
  }
  else {
    strncpy(out_url->user, URL_DEFAULT_USERNAME, sizeof(out_url->user));
    strncpy(out_url->pass, URL_DEFAULT_PASSWORD, sizeof(out_url->pass));
  }

  //host, optional port
  if(regexec(&hostport_re, tmp, 1, match, 0) != 0) {
    error2("error parsing out host and optional port, returning\n");
    goto parse_url_die;
  }
  else {
    //copy path first (saves space)
    if(match[0].rm_so != 0)
      error2("SHIT!  match[0].rm_so != 0\n");
    tok= match[0].rm_eo + tmp;
    strncpy(out_url->path, (*tok == 0x0)? "": tok, sizeof(out_url->path));
    *tok= 0x0;

    strncpy(out_url->host, strtok_mw(tmp, ":", &ptr), sizeof(out_url->host));
    tok= strtok_mw(NULL, "", &ptr);
    if(tok != NULL && *tok != 0x0)
      out_url->port= atoi(tok);
    else if(strncasecmp(out_url->proto, "http", sizeof(out_url->proto)) == 0)
      out_url->port= 80;  //default http port
    else if(strncasecmp(out_url->proto, "ftp", sizeof(out_url->proto)) == 0)
      out_url->port= 21;  //default ftp port
    else {
      debug("setting out_url->port to 0 - [%s] is an unknown protocol\n", out_url->proto);
      out_url->port= 0;
    }
  }

  free(urlcpy);
  debug("parse_url(%s) succeeded\n", out_url->host);
  return 0;

 parse_url_die:
  free(urlcpy);
  return -1;
}

int parse_http_pages(list_t *src_list, list_t *dest_list)
{
  int sock, i;
  regex_t ftp_re;

  if(list_count(src_list) < 1) {
    debug("list_count(src_list) < 1, returning successfully\n");
    return 0;
  }

  if(regcomp(&ftp_re, "ftp://("legal"+(:"legal"+)?@)?"legal"+(:[0-9]+)?(/"legal"*)*",
             REG_EXTENDED | REG_ICASE | REG_NEWLINE) != 0) {
    error2("regcomp() failed, returning [%s]\n", strerror(errno));
    return -1;
  }

  for(i=0; i<list_count(src_list); i++) {
    struct sockaddr_in addr;
    regmatch_t match;
    char buff[BUFFSZ], *url;
    url_t httpurl;
    struct hostent *tmpent;

    url= (char *)list_get(src_list, i);
    if(url == NULL)
      continue;

    if(parse_url(url, &httpurl) != 0) {
      error2("parse_url() failed, skipping\n");
      continue;
    }

    sock= socket(PF_INET, SOCK_STREAM, 0);
    if(sock == -1) {
      error2("socket() failed, skipping - [%s]\n", strerror(errno));
      continue;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family= AF_INET;
    addr.sin_port= htons(httpurl.port);
    tmpent= gethostbyname(httpurl.host);
    if(tmpent == NULL) {
      error2("gethostbyname() failed, skipping\n");
      continue;
    }
    addr.sin_addr= *(struct in_addr *)tmpent->h_addr_list[0];

    if(connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
      error2("connect() failed, skipping - [%s]\n", strerror(errno));
      continue;
    }

    /*
    sock_stream= fdopen(sock, "r+");
    if(sock_stream == NULL) {
      error2("fdopen() failed, skipping - [%s]\n", strerror(errno));
      close(sock);
      continue;
    }
    */

    /*
    fprintf(sock_stream, "GET %s\n", url);
    while(!feof(sock_stream)) {
      if(fgets(buff, sizeof(buff), sock_stream) == NULL)
        continue;
    */

    //TODO: I tried to get away from using fdopen(), but this method might have
    // problems in that it will possible lose a url if the url is split
    // between lines.  This should eventually be fixed.

    sprintf(buff, "GET %s\n", url);
    send(sock, buff, strlen(buff), 0);
    while(1) {
      int ret;
      char *tok, *ptr;

      if((ret= recv(sock, buff, sizeof(buff), 0)) < 1)
	break;
      if((tok= strtok_mw(buff, "\n", &ptr)) == NULL)
	continue;

      do {
	//TODO: only matches one per line right now
	int cur= 0;
	while(regexec(&ftp_re, cur + tok, 1, &match, 0) == 0) {
	  int len= match.rm_eo - match.rm_so;
	  char *tmp= malloc(1 + len);
	  memcpy(tmp, match.rm_so + cur + tok, len);
	  tmp[len]= 0x0;
	  if(list_add_unique_with_comparator(dest_list, tmp, char_ptr_comparator) != 0) {
	    debug("duplicate ftp url [%s] not added to list\n", tmp);
	    free(tmp);
	  }
	  else
	    debug("added [%s] to list\n", tmp);
	  cur= cur + match.rm_eo;
	}
      } while((tok= strtok_mw(NULL, "\n", &ptr)) != NULL);
    }

    close(sock);
  }

  regfree(&ftp_re);

  return 0;
}

char *strtok_mw(char *s, const char *delim, char **ptrptr)
{
  return (char *)strtok_r(s, delim, ptrptr);
}
