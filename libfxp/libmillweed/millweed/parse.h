
#ifndef LIBMILLWEED_PARSE_H
#define LIBMILLWEED_PARSE_H


#include <millweed/misc.h>
#include <millweed/list.h>

#ifdef __cplusplus
extern "C" {
#endif

#define URL_DEFAULT_USERNAME  "anonymous"
#define URL_DEFAULT_PASSWORD  "nobody@localhost.localdomain"

//generic url parsing
typedef struct url_s {
  char proto[SMBUFFSZ];

  char host[SMBUFFSZ];
  int port;
  char path[BUFFSZ];

  char user[SMBUFFSZ];
  char pass[SMBUFFSZ];
} url_t;

int parse_url(const char *url_string, url_t *out_url);
int url_to_string(const url_t *url, char *out_buff, unsigned out_buff_size);

//http page parsing (for ftp urls right now)
int parse_http_pages(list_t *src_list, list_t *dest_list);

//Solaris had problems with strtok_r
extern char *strtok_mw(char *s, const char *delim, char **ptrptr);

#ifdef __cplusplus
}
#endif


#endif
