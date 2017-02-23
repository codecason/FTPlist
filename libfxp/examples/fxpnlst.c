
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <fxp.h>

int verbosity= VERBOSITY_DEBUG;

fxp_error_t the_hook(fxp_handle_t fxp, const char *message, unsigned message_len)
{
  fprintf(stderr, CYELLOW "%s" COFF, message);
  fflush(stderr);

  return FXPOK;
}

int main(int argc, char *argv[])
{
  url_t url;
  fxp_handle_t fxp;
  list_t *files= list_create();
  int i;

  if(argc != 2) {
    fprintf(stderr, "usage: %s ftp_url\n", argv[0]);
    return -1;
  }
  if(parse_url(argv[1], &url) != 0) {
    error("parse_url() failed\n");
    return -1;
  }
  if(*url.path == 0x0)
    strncpy(url.path, "/", sizeof(url.path));
  if(fxp_init(&fxp, &url) != FXPOK) {
    error("fxp_init() failed\n");
    return -1;
  }
  fxp_set_control_send_hook(fxp, the_hook);
  fxp_set_control_receive_hook(fxp, the_hook);
  if(fxp_connect(fxp, 10000) != FXPOK) {
    error("fxp_connect() failed\n");
    goto die;
  }
  if(fxp_login(fxp, NULL, NULL) != FXPOK) {
    error("fxp_login() failed\n");
    goto die;
  }

  fxp_mode(fxp, FXP_MODE_PASSIVE);
  if(fxp_nlst(fxp, url.path, files) != FXPOK) {
    error("fxp_nlst() failed\n");
    goto die;
  }

  for(i=0; i<list_count(files); i++) {
    fxp_file_t *ptr= list_get(files, i);

    switch(ptr->type) {
    case FXP_FILE_TYPE_DIRECTORY:
      mess("[dir]  " CGREEN "%s" COFF "\n", ptr->name);
      break;
    case FXP_FILE_TYPE_LINK:
      mess("[link] " CGREEN "%s" COFF "\n", ptr->name);
      break;
    case FXP_FILE_TYPE_REGULAR:
      mess("[file] " CGREEN "%s" COFF "\n", ptr->name);
      break;
    default:
      error("[unk]  " CGREEN "%s" COFF "\n", ptr->name);
    }
  }

  list_destroy(files);
  fxp_disconnect(fxp);
  fxp_destroy(&fxp);

  return 0;

 die:
  list_destroy(files);
  fxp_disconnect(fxp);
  fxp_destroy(&fxp);

  return -1;
}
