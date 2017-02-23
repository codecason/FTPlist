
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "fxp_private.h"

fxp_error_t fxp_pwd(fxp_handle_t fxp, char *out_dirname, unsigned out_dirname_size)
{
  char *rest= NULL, *path, *tmp;
  unsigned code;
  fxp_error_t fxpret;

  if(fxp == NULL || out_dirname == NULL)
    return -FXPENULLP;
  if(out_dirname_size == 0 || fxp->ctrl_sock == 0)
    return -FXPEINVAL;

  if(fxp->pwd_cache == NULL) {
    if((fxpret= fxp_ctrl_send_message(fxp, "PWD" FXPEOL)) != FXPOK) {
      error2("fxp_ctrl_send_message(PWD) failed, returning\n");
      return fxpret;
    }
    if((fxpret= fxp_ctrl_receive_code(fxp, &code, &rest)) != FXPOK) {
      error2("fxp_ctrl_receive_code() failed, returning\n");
      return fxpret;
    }
    if(code == 421) {
      error2("PWD 421, disconnecting and returning\n");
      if(fxp_disconnect_hard(fxp) != FXPOK)
	error2("fxp_disconnect_hard() failed, ignoring\n");
      return -FXPECLOSED;
    }
    if(code == 550) {
      error2("PWD file not found, returning\n");
      return -FXPENOTFOUND;
    }
    if(code != 257) {
      error2("PWD unknown code [%u], returning\n", code);
      return -FXPEUNKNOWN;
    }
    debug("PWD succeeded\n");

    if((path= strchr(rest, '\"')) == NULL || *(++path) == 0x0) {
      error2("error parsing response, returning\n");
      return -FXPEINVAL;
    }
    if((tmp= strchr(path, '\"')) == NULL || *tmp == 0x0) {
      error2("error parsing response, returning\n");
      return -FXPEINVAL;
    }
    *tmp= 0x0;

    fxp->pwd_cache= strdup(path);
  }

  strncpy(out_dirname, fxp->pwd_cache, out_dirname_size);

  debug("fxp_pwd() succeeded\n");
  return FXPOK;
}

fxp_error_t fxp_cdup(fxp_handle_t fxp)
{
  fxp_error_t fxpret;

  if((fxpret= fxp_cwd(fxp, "..")) != FXPOK)
    return fxpret;

  debug("fxp_cdup() succeeded\n");
  return FXPOK;
}

fxp_error_t fxp_cwd(fxp_handle_t fxp, const char *new_dirname)
{
  char cwd_cmd[SMBUFFSZ], *rest= NULL, *path, *tmp;
  unsigned codes[2]= {}, codes_size= 2;
  fxp_error_t fxpret;

  if(fxp == NULL || new_dirname == NULL)
    return -FXPENULLP;
  if(*new_dirname == 0x0 || fxp->ctrl_sock == 0)
    return -FXPEINVAL;
  if(strcmp(new_dirname, ".") == 0 || strcmp(new_dirname, "./") == 0)
    return -FXPENOCHANGE;

  if(strcasecmp(new_dirname, "..") == 0 || strcasecmp(new_dirname, "../") == 0) {
    strcpy(cwd_cmd, "CDUP");
    fxpret= fxp_ctrl_send_message(fxp, "CDUP" FXPEOL "PWD" FXPEOL);
  }
  else {
    strcpy(cwd_cmd, "CWD");
    fxpret= fxp_ctrl_send_message(fxp, "CWD %s" FXPEOL "PWD" FXPEOL, new_dirname);
  }
  if(fxpret != FXPOK) {
    error2("fxp_ctrl_send_message(%s+PWD) failed, returning\n", cwd_cmd);
    return fxpret;
  }

  if((fxpret= fxp_ctrl_receive_codes(fxp, codes, &codes_size, &rest)) == FXPOK && codes_size < 2)
    fxpret= fxp_ctrl_receive_codes(fxp, codes+1, &codes_size, &rest);  //get second code if needed
  if(fxpret != FXPOK) {
    error2("%s+PWD fxp_ctrl_receive_codes() failed, returning\n", cwd_cmd);
    return fxpret;
  }

  if(codes[0] == 421) {
    error2("%s 421, disconnecting and returning\n", cwd_cmd);
    if(fxp_disconnect_hard(fxp) != FXPOK)
      error2("fxp_disconnect_hard() failed, ignoring\n");
    return -FXPECLOSED;
  }
  if(codes[0] == 550) {
    error2("%s file not found, returning\n", cwd_cmd);
    return -FXPENOTFOUND;
  }
  if(codes[0] != 250 && codes[0] != 150) {
    error2("%s unknown code [%u], returning\n", cwd_cmd, codes[0]);
    return -FXPEUNKNOWN;
  }
  debug("%s succeeded\n", cwd_cmd);

  if(fxp->pwd_cache != NULL) {
    free(fxp->pwd_cache);
    fxp->pwd_cache= NULL;
  }
  if(codes[1] == 421) {
    error2("PWD 421, disconnecting and returning\n");
    if(fxp_disconnect_hard(fxp) != FXPOK)
      error2("fxp_disconnect_hard() failed, ignoring\n");
    return -FXPECLOSED;
  }
  if(codes[1] == 550) {
    error2("PWD file not found, returning\n");
    return -FXPENOTFOUND;
  }
  if(codes[1] != 257) {
    error2("PWD unknown code [%u], returning\n", codes[1]);
    return -FXPEUNKNOWN;
  }
  debug("PWD succeeded\n");

  if((path= strchr(rest, '\"')) == NULL || *(++path) == 0x0) {
    error2("error parsing response, returning\n");
    return -FXPEINVAL;
  }
  if((tmp= strchr(path, '\"')) == NULL || *tmp == 0x0) {
    error2("error parsing response, returning\n");
    return -FXPEINVAL;
  }
  *tmp= 0x0;
  fxp->pwd_cache= strdup(path);

  debug("fxp_cwd() succeeded\n");
  return FXPOK;
}
