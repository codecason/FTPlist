
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>

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
  char buff[10240], *filename;
  FILE *fh;
  struct stat st;
  fxp_error_t fxpret;

  if(argc != 2) {
    fprintf(stderr, "usage: %s ftp_url\n", argv[0]);
    return -1;
  }
  if(parse_url(argv[1], &url) != 0) {
    error("parse_url() failed\n");
    return -1;
  }
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
  fxp_type(fxp, FXP_TYPE_BINARY);

  if((filename= strrchr(url.path, '/')) != NULL)
    filename++;
  else
    filename= url.path;
  if(stat(filename, &st) == 0)
    fxpret= fxp_retr_rest(fxp, url.path, st.st_size);
  else
    fxpret= fxp_retr(fxp, url.path);
  if(fxpret != FXPOK) {
    error("fxp_retr*() failed\n");
    goto die;
  }

  if((fh= fopen(filename, "a")) == NULL) {
    error("fopen(%s) failed\n", filename);
    goto die;
  }

  while(1) {
    unsigned buff_size= sizeof(buff);
    fxp_error_t fxpret;

    if((fxpret= fxp_retr_read(fxp, buff, &buff_size)) == FXPOK) {
      mess2("read [%u] bytes\n", buff_size);
      fwrite(buff, 1, buff_size, fh);
    }

    if(fxpret != FXPOK || buff_size < sizeof(buff))
      break;
  }

  //fxp_retr_close(fxp);  //this is implicitly called when fxp_retr_read() encounters EOF
  fclose(fh);
  fxp_disconnect(fxp);
  fxp_destroy(&fxp);

  return 0;

 die:
  fxp_disconnect(fxp);
  fxp_destroy(&fxp);

  return -1;
}
