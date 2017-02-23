
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "fxp_private.h"

#define DEFAULT_TIMEOUT  60000

fxp_error_t fxp_init(fxp_handle_t *pfxp, const url_t *url)
{
  fxp_handle_t fxp= NULL;

  if(pfxp == NULL || url == NULL)
    return -FXPENULLP;
  if(strlen(url->host) < 1)
    return -FXPEINVAL;

  if((fxp= malloc(sizeof(fxp_t))) == NULL)
    return -FXPEMEMORY;
  memset(fxp, 0, sizeof(fxp_t));

  memcpy(&fxp->url, url, sizeof(url_t));
  fxp->ctrl_recv_timeo= DEFAULT_TIMEOUT;
  fxp->data_estab_timeo= DEFAULT_TIMEOUT;
  fxp->data_recv_timeo= DEFAULT_TIMEOUT;
  fxp->data_send_timeo= DEFAULT_TIMEOUT;

  fxp->data_mode= FXP_MODE_PASSIVE;

  if(strncasecmp(fxp->url.proto, "ftp", sizeof(fxp->url.proto)) != 0) {  //proto check
    error2("url->proto != \"ftp\", ignoring\n");
    strncpy(fxp->url.proto, "ftp", sizeof(fxp->url.proto));
  }

  *pfxp= fxp;

  debug("fxp_init() succeeded\n");
  return FXPOK;
}

fxp_error_t fxp_destroy(fxp_handle_t *pfxp)
{
  if(pfxp == NULL || *pfxp == NULL)
    return -FXPENULLP;

  //TODO: maybe make an implicit call to fxp_disconnect()?

  if((*pfxp)->pwd_cache != NULL)
     free((*pfxp)->pwd_cache);

  free(*pfxp);
  *pfxp= NULL;

  debug("fxp_destroy() succeeded\n");
  return FXPOK;
}
