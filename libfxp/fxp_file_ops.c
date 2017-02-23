
#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "fxp_private.h"

fxp_error_t fxp_rename(fxp_handle_t fxp, const char *src_filename, const char *dst_filename)
{
  unsigned code;
  fxp_error_t fxpret;

  if(fxp == NULL || src_filename == NULL || dst_filename == NULL)
    return -FXPENULLP;
  if(*src_filename == 0x0 || *dst_filename == 0x0 || fxp->ctrl_sock == 0)
    return -FXPEINVAL;

  if((fxpret= fxp_ctrl_send_message(fxp, "RNFR %s" FXPEOL, src_filename)) != FXPOK) {
    error2("fxp_ctrl_send_message(RNFR) failed, returning\n");
    return fxpret;
  }
  if((fxpret= fxp_ctrl_receive_code(fxp, &code, NULL)) != FXPOK) {
    error2("fxp_ctrl_receive_code() failed, returning\n");
    return fxpret;
  }
  if(code == 421) {
    error2("RNFR 421, disconnecting and returning\n");
    if(fxp_disconnect_hard(fxp) != FXPOK)
      error2("fxp_disconnect_hard() failed, ignoring\n");
    return -FXPECLOSED;
  }
  if(code == 550) {
    error2("RNFR file not found, returning\n");
    return -FXPENOTFOUND;
  }
  if(code != 350) {
    error2("RNFR unknown code [%u], returning\n", code);
    return -FXPEUNKNOWN;
  }
  debug("RNFR succeeded, ready for RNTO\n");

  if((fxpret= fxp_ctrl_send_message(fxp, "RNTO %s" FXPEOL, dst_filename)) != FXPOK) {
    error2("fxp_ctrl_send_message(RNTO) failed, returning\n");
    return fxpret;
  }
  if((fxpret= fxp_ctrl_receive_code(fxp, &code, NULL)) != FXPOK) {
    error2("fxp_ctrl_receive_code() failed, returning\n");
    return fxpret;
  }
  if(code == 421) {
    error2("RNTO 421, disconnecting and returning\n");
    if(fxp_disconnect_hard(fxp) != FXPOK)
      error2("fxp_disconnect_hard() failed, ignoring\n");
    return -FXPECLOSED;
  }
  if(code == 550) {
    error2("RNTO failed, returning\n");
    return -FXPEUNKNOWN;
  }
  if(code == 553) {
    error2("RNTO permission denied, returning\n");
    return -FXPEACCESS;
  }
  if(code != 250) {
    error2("RNTO unknown code [%u], returning\n", code);
    return -FXPEUNKNOWN;
  }
  debug("RNTO succeeded\n");

  debug("fxp_rename() succeeded\n");
  return FXPOK;
}

fxp_error_t fxp_dele(fxp_handle_t fxp, const char *filename)
{
  unsigned code;
  fxp_error_t fxpret;

  if(fxp == NULL || filename == NULL)
    return -FXPENULLP;
  if(*filename == 0x0 || fxp->ctrl_sock == 0)
    return -FXPEINVAL;

  if((fxpret= fxp_ctrl_send_message(fxp, "DELE %s" FXPEOL, filename)) != FXPOK) {
    error2("fxp_ctrl_send_message(DELE) failed, returning\n");
    return fxpret;
  }
  if((fxpret= fxp_ctrl_receive_code(fxp, &code, NULL)) != FXPOK) {
    error2("fxp_ctrl_receive_code() failed, returning\n");
    return fxpret;
  }
  if(code == 421) {
    error2("DELE 421, disconnecting and returning\n");
    if(fxp_disconnect_hard(fxp) != FXPOK)
      error2("fxp_disconnect_hard() failed, ignoring\n");
    return -FXPECLOSED;
  }
  if(code == 550) {
    error2("DELE file not found, returning\n");
    return -FXPENOTFOUND;
  }
  if(code == 553) {
    error2("DELE permission denied, returning\n");
    return -FXPEACCESS;
  }
  if(code != 250) {
    error2("DELE unknown code [%u], returning\n", code);
    return -FXPEUNKNOWN;
  }
  debug("DELE succeeded\n");

  debug("fxp_delete() succeeded\n");
  return FXPOK;
}

fxp_error_t fxp_mkd(fxp_handle_t fxp, const char *dirname)
{
  unsigned code;
  fxp_error_t fxpret;

  if(fxp == NULL || dirname == NULL)
    return -FXPENULLP;
  if(*dirname == 0x0 || fxp->ctrl_sock == 0)
    return -FXPEINVAL;

  if((fxpret= fxp_ctrl_send_message(fxp, "MKD %s" FXPEOL, dirname)) != FXPOK) {
    error2("fxp_ctrl_send_message(MKD) failed, returning\n");
    return fxpret;
  }
  if((fxpret= fxp_ctrl_receive_code(fxp, &code, NULL)) != FXPOK) {
    error2("fxp_ctrl_receive_code() failed, returning\n");
    return fxpret;
  }
  if(code == 421) {
    error2("MKD 421, disconnecting and returning\n");
    if(fxp_disconnect_hard(fxp) != FXPOK)
      error2("fxp_disconnect_hard() failed, ignoring\n");
    return -FXPECLOSED;
  }
  if(code == 521) {
    error2("MKD directory exists, returning\n");
    return -FXPENOCHANGE;
  }
  if(code == 550 || code == 553) {
    error2("MKD permission denied, returning\n");
    return -FXPEACCESS;
  }
  if(code != 257) {
    error2("MKD unknown code [%u], returning\n", code);
    return -FXPEUNKNOWN;
  }
  debug("MKD succeeded\n");

  debug("fxp_mkdir() succeeded\n");
  return FXPOK;
}

fxp_error_t fxp_rmd(fxp_handle_t fxp, const char *dirname)
{
  unsigned code;
  fxp_error_t fxpret;

  if(fxp == NULL || dirname == NULL)
    return -FXPENULLP;
  if(*dirname == 0x0 || fxp->ctrl_sock == 0)
    return -FXPEINVAL;

  if((fxpret= fxp_ctrl_send_message(fxp, "RMD %s" FXPEOL, dirname)) != FXPOK) {
    error2("fxp_ctrl_send_message(RMD) failed, returning\n");
    return fxpret;
  }
  if((fxpret= fxp_ctrl_receive_code(fxp, &code, NULL)) != FXPOK) {
    error2("fxp_ctrl_receive_code() failed, returning\n");
    return fxpret;
  }
  if(code == 421) {
    error2("RMD 421, disconnecting and returning\n");
    if(fxp_disconnect_hard(fxp) != FXPOK)
      error2("fxp_disconnect_hard() failed, ignoring\n");
    return -FXPECLOSED;
  }
  if(code == 550) {
    error2("RMD file not found, returning\n");
    return -FXPENOTFOUND;
  }
  if(code == 553) {
    error2("RMD permission denied, returning\n");
    return -FXPEACCESS;
  }
  if(code != 250) {
    error2("RMD unknown code [%u], returning\n", code);
    return -FXPEUNKNOWN;
  }
  debug("RMD succeeded\n");

  debug("fxp_rmdir() succeeded\n");
  return FXPOK;
}

fxp_error_t fxp_stat(fxp_handle_t fxp, const char *filename, fxp_complete_file_t *out_file)
{
  list_t *list;
  fxp_file_t *ret_file= NULL;
  char *name_cpy, *str;
  int i;
  fxp_error_t fxpret;

  if(fxp == NULL || filename == NULL || out_file == NULL)
    return -FXPENULLP;

  if(*filename == 0x0 || strchr(filename, '*') != NULL || fxp->ctrl_sock == 0)
    return -FXPEINVAL;

  name_cpy= strdup(filename);
  if(strcmp(name_cpy, "/") != 0) {
    char *ptr= name_cpy + strlen(name_cpy)-1;
    while(ptr >= name_cpy && *ptr == '/')
      *(ptr--)= 0x0;  //get rid of trailing '/'s
  }

  list= list_create();

  str= malloc(strlen(name_cpy)+2);
  sprintf(str, "%s*", name_cpy);
  fxpret= fxp_expand_wildcards(fxp, str, list);
  free(str);

  if(fxpret != FXPOK) {
    error2("fxp_expand_wildcards() failed, returning\n");
    list_destroy(list);
    free(name_cpy);
    return fxpret;
  }

  str= strrchr(name_cpy, '/');
  str= (str == NULL)? name_cpy: str+1;
  for(i=0; i<list_count(list) && ret_file == NULL; i++) {
    fxp_file_t *file= (fxp_file_t *)list_get(list, i);
    if(strcmp(file->name, str) == 0)
      ret_file= file;
  }
  if(ret_file == NULL) {
    error2("stat not found for file, returning\n");
    list_destroy(list);
    free(name_cpy);
    return -FXPENOTFOUND;
  }

  memcpy(out_file, ret_file, (ret_file->is_complete)? sizeof(fxp_complete_file_t): sizeof(fxp_file_t));
  strncpy(out_file->name, name_cpy, sizeof(ret_file->name));  //name might be off
  list_destroy(list);

  debug("fxp_stat() succeeded\n");
  return FXPOK;
}
