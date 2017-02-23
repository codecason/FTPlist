
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "fxp_private.h"

fxp_error_t fxp_file_close(fxp_handle_t fxp, unsigned *out_code)
{
  unsigned code;
  fxp_error_t fxpret;

  if(fxp == NULL)
    return -FXPENULLP;
  if(fxp->ctrl_sock == 0)
    return -FXPEINVAL;
  if(fxp->file_open == 0)
    return -FXPENOCHANGE;

  code= fxp->last_xfer_code;
  if(fxp_data_cleanup(fxp) != FXPOK)
    error2("fxp_data_cleanup() failed, ignoring\n");

  if(code == 0) {
    if((fxpret= fxp_ctrl_receive_code(fxp, &code, NULL)) != FXPOK) {
      error2("fxp_ctrl_receive_code() failed, returning\n");
      return fxpret;
    }
    if(code == 421) {
      error2("fxp_file_close() 421, disconnecting and returning\n");
      if(fxp_disconnect_hard(fxp) != FXPOK)
	error2("fxp_disconnect_hard() failed, ignoring\n");
      return -FXPECLOSED;
    }
  }
  if(out_code != NULL)
    *out_code= code;

  debug("fxp_file_close() succeeded\n");
  return FXPOK;
}

fxp_error_t fxp_rest(fxp_handle_t fxp, unsigned long offset)
{
  fxp_error_t fxpret;
  unsigned code;

  if(fxp == NULL)
    return -FXPENULLP;
  if(fxp->ctrl_sock == 0)
    return -FXPEINVAL;
  if(offset < 1)
    return -FXPENOCHANGE;

  if((fxpret= fxp_ctrl_send_message(fxp, "REST %u" FXPEOL, offset)) != FXPOK) {
    error2("fxp_ctrl_send_message(REST) failed, returning\n");
    if(fxp_data_cleanup(fxp) != FXPOK)
      error2("fxp_data_cleanup() failed, ignoring\n");
    return fxpret;
  }
  if((fxpret= fxp_ctrl_receive_code(fxp, &code, NULL)) != FXPOK) {
    error2("fxp_ctrl_receive_code() failed, returning\n");
    return fxpret;
  }
  if(code == 421) {
    error2("REST 421, disconnecting and returning\n");
    if(fxp_disconnect_hard(fxp) != FXPOK)
      error2("fxp_disconnect_hard() failed, ignoring\n");
    return -FXPECLOSED;
  }
  if(code == 502) {
    error2("REST unsupported, returning\n");
    return -FXPEUNSUPP;
  }
  if(code != 350) {
    error2("REST unknown code [%u], returning\n", code);
    return -FXPEUNKNOWN;
  }
  debug("REST succeeded\n");

  debug("fxp_rest() succeeded\n");
  return FXPOK;
}

fxp_error_t fxp_retr(fxp_handle_t fxp, const char *filename)
{
  return fxp_retr_rest(fxp, filename, 0);
}

fxp_error_t fxp_retr_rest(fxp_handle_t fxp, const char *filename, unsigned long offset)
{
  unsigned codes[2], num_codes= sizeof(codes)/sizeof(codes[0]);
  fxp_error_t fxpret;
  char *rest= NULL;

  if(fxp == NULL || filename == NULL)
    return -FXPENULLP;
  if(*filename == 0x0 || fxp->ctrl_sock == 0 || fxp->file_open != 0)
    return -FXPEINVAL;

  if((fxpret= fxp_data_establish_setup(fxp)) != FXPOK) {
    error2("fxp_data_establish_setup() failed, returning\n");
    return fxpret;
  }

  if(offset > 0) {
    if((fxpret= fxp_rest(fxp, offset)) != FXPOK) {
      error2("fxp_rest() failed, returning\n");
      if(fxp_data_cleanup(fxp) != FXPOK)
	error2("fxp_data_cleanup() failed, ignoring\n");
      return fxpret;
    }
  }

  if((fxpret= fxp_ctrl_send_message(fxp, "RETR %s" FXPEOL, filename)) != FXPOK) {
    error2("fxp_ctrl_send_message(RETR) failed, returning\n");
    if(fxp_data_cleanup(fxp) != FXPOK)
      error2("fxp_data_cleanup() failed, ignoring\n");
    return fxpret;
  }

  if((fxpret= fxp_data_establish(fxp)) != FXPOK) {
    if(fxpret != -FXPEINTERRUPT) {
      error2("fxp_data_establish() failed, returning\n");
      if(fxp_data_cleanup(fxp) != FXPOK)
	error2("fxp_data_cleanup() failed, ignoring\n");
      return fxpret;
    }
    debug("fxp_data_establish() interrupted, continuing()\n");
  }

  if((fxpret= fxp_ctrl_receive_codes(fxp, codes, &num_codes, &rest)) != FXPOK) {
    error2("fxp_ctrl_receive_codes() failed, returning\n");
    if(fxp_data_cleanup(fxp) != FXPOK)
      error2("fxp_data_cleanup() failed, ignoring\n");
    return fxpret;
  }

  //save the remote file size in the response
  fxp->retr_size= 0;
  if(rest != NULL && (rest= strrchr(rest, '(')) != NULL) {
    char *ptr, *tok;

    if((tok= strtok_mw(++rest, " ", &ptr)) != NULL)
      fxp->retr_size= strtoul(tok, NULL, 10);
  }

  if(codes[0] == 421) {
    error2("RETR 421, disconnecting and returning\n");
    if(fxp_disconnect_hard(fxp) != FXPOK)
      error2("fxp_disconnect_hard() failed, ignoring\n");
    return -FXPECLOSED;
  }
  if(codes[0] == 550) {
    error2("RETR file not found, returning\n");
    if(fxp_data_cleanup(fxp) != FXPOK)
      error2("fxp_data_cleanup() failed, ignoring\n");
    return -FXPENOTFOUND;
  }
  if(codes[0] == 425) {
    error2("RETR couldn't build data connection, returning\n");
    if(fxp_data_cleanup(fxp) != FXPOK)
      error2("fxp_data_cleanup() failed, ignoring\n");
    return -FXPEUNKNOWN;
  }
  if(codes[0] != 150 && codes[0] != 125) {
    error2("RETR unknown code [%u], returning\n", codes[0]);
    if(fxp_data_cleanup(fxp) != FXPOK)
      error2("fxp_data_cleanup() failed, ignoring\n");
    return -FXPEUNKNOWN;
  }
  if(num_codes > 1 && codes[1] == 226) {  //wow, both codes gotten before transfer, that's ok
    fxp->last_xfer_code= codes[1];
    debug("RETR transfer already completed, nice\n");
  }
  else
    debug("RETR initiated\n");

  fxp->file_open= 1;

  return FXPOK;
}

fxp_error_t fxp_retr_get_size(fxp_handle_t fxp, unsigned long long *out_size)
{
  if(fxp == NULL || out_size == NULL)
    return -FXPENULLP;
  if(fxp->ctrl_sock == 0 || fxp->retr_size < 1)
    return -FXPEINVAL;

  *out_size= fxp->retr_size;
  return FXPOK;
}

fxp_error_t fxp_retr_close(fxp_handle_t fxp)
{
  fxp_error_t fxpret;
  unsigned code;

  if((fxpret= fxp_file_close(fxp, &code)) != FXPOK && fxpret != -FXPENOCHANGE) {
    error2("fxp_file_close() failed, returning\n");
    return fxpret;
  }
  else if(fxpret != -FXPENOCHANGE && code != 226) {
    error2("RETR unknown code [%u], returning\n", code);
    return -FXPEUNKNOWN;
  }
  debug("RETR succeeded\n");

  debug("fxp_retr_close() succeeded\n");
  return FXPOK;
}

fxp_error_t fxp_retr_read(fxp_handle_t fxp, char *out_buff, unsigned *out_buff_size)
{
  unsigned cur_buff_size= 0, buff_size= 0;
  fxp_error_t fxpret;

  if(fxp == NULL || out_buff == NULL || out_buff_size == NULL)
    return -FXPENULLP;
  if(fxp->ctrl_sock == 0 || fxp->data_sock == 0 || *out_buff_size < 1 || fxp->file_open != 1)
    return -FXPEINVAL;

  while(cur_buff_size < *out_buff_size) {
    buff_size= *out_buff_size - cur_buff_size;

    if((fxpret= fxp_data_receive(fxp, out_buff + cur_buff_size, &buff_size)) != FXPOK)
      break;
    cur_buff_size += buff_size;
  }

  if(fxpret != FXPOK || cur_buff_size < *out_buff_size)  //end of file
    if(fxp_retr_close(fxp) != FXPOK)
      error2("fxp_retr_close() failed, ignoring\n");

  if(fxpret != FXPOK && fxpret != -FXPETIMEOUT)
    return fxpret;

  *out_buff_size= cur_buff_size;

  debug("fxp_retr_read() succeeded - read [%u] bytes\n", *out_buff_size);
  return FXPOK;
}

fxp_error_t fxp_stor(fxp_handle_t fxp, const char *filename)
{
  return fxp_stor_rest(fxp, filename, 0);
}

fxp_error_t fxp_stor_rest(fxp_handle_t fxp, const char *filename, unsigned long offset)
{
  unsigned codes[2], num_codes= sizeof(codes)/sizeof(codes[0]);
  fxp_error_t fxpret;

  if(fxp == NULL || filename == NULL)
    return -FXPENULLP;
  if(*filename == 0x0 || fxp->ctrl_sock == 0 || fxp->file_open != 0)
    return -FXPEINVAL;

  if((fxpret= fxp_data_establish_setup(fxp)) != FXPOK) {
    error2("fxp_data_establish_setup() failed, returning\n");
    return fxpret;
  }

  if(offset > 0) {
    if((fxpret= fxp_rest(fxp, offset)) != FXPOK) {
      error2("fxp_rest() failed, returning\n");
      if(fxp_data_cleanup(fxp) != FXPOK)
	error2("fxp_data_cleanup() failed, ignoring\n");
      return fxpret;
    }
  }

  if((fxpret= fxp_ctrl_send_message(fxp, "STOR %s" FXPEOL, filename)) != FXPOK) {
    error2("fxp_ctrl_send_message(STOR) failed, returning\n");
    if(fxp_data_cleanup(fxp) != FXPOK)
      error2("fxp_data_cleanup() failed, ignoring\n");
    return fxpret;
  }

  if((fxpret= fxp_data_establish(fxp)) != FXPOK) {
    if(fxpret != -FXPEINTERRUPT) {
      error2("fxp_data_establish() failed, returning\n");
      if(fxp_data_cleanup(fxp) != FXPOK)
	error2("fxp_data_cleanup() failed, ignoring\n");
      return fxpret;
    }
    debug("fxp_data_establish() interrupted, continuing()\n");
  }

  if((fxpret= fxp_ctrl_receive_codes(fxp, codes, &num_codes, NULL)) != FXPOK) {
    error2("fxp_ctrl_receive_codes() failed, returning\n");
    if(fxp_data_cleanup(fxp) != FXPOK)
      error2("fxp_data_cleanup() failed, ignoring\n");
    return fxpret;
  }
  if(codes[0] == 421) {
    error2("STOR 421, disconnecting and returning\n");
    if(fxp_disconnect_hard(fxp) != FXPOK)
      error2("fxp_disconnect_hard() failed, ignoring\n");
    return -FXPECLOSED;
  }
  if(codes[0] == 553) {
    error2("STOR permission denied, returning\n");
    if(fxp_data_cleanup(fxp) != FXPOK)
      error2("fxp_data_cleanup() failed, ignoring\n");
    return -FXPEACCESS;
  }
  if(codes[0] == 425) {
    error2("STOR couldn't build data connection, returning\n");
    if(fxp_data_cleanup(fxp) != FXPOK)
      error2("fxp_data_cleanup() failed, ignoring\n");
    return -FXPEUNKNOWN;
  }
  if(codes[0] != 150 && codes[0] != 125) {
    error2("STOR unknown code [%u], returning\n", codes[0]);
    if(fxp_data_cleanup(fxp) != FXPOK)
      error2("fxp_data_cleanup() failed, ignoring\n");
    return -FXPEUNKNOWN;
  }
  if(num_codes > 1 && codes[1] == 226) {  //wow, both codes gotten before transfer, that's ok
    fxp->last_xfer_code= codes[1];
    debug("STOR transfer already completed, nice\n");
  }
  else
    debug("STOR initiated\n");

  fxp->file_open= 2;

  return FXPOK;
}

fxp_error_t fxp_stor_close(fxp_handle_t fxp)
{
  unsigned code;
  fxp_error_t fxpret;

  if((fxpret= fxp_file_close(fxp, &code)) != FXPOK && fxpret != -FXPENOCHANGE) {
    error2("fxp_file_close() failed, returning\n");
    return fxpret;
  }
  else if(fxpret != -FXPENOCHANGE && code != 226) {
    error2("STOR unknown code [%u], returning\n", code);
    return -FXPEUNKNOWN;
  }
  debug("STOR succeeded\n");

  debug("fxp_stor_close() succeeded\n");
  return FXPOK;
}

fxp_error_t fxp_stor_write(fxp_handle_t fxp, const char *buff, unsigned buff_size)
{
  unsigned num_sent= 0;
  fxp_error_t fxpret;

  if(fxp == NULL || buff == NULL)
    return -FXPENULLP;
  if(fxp->ctrl_sock == 0 || fxp->data_sock == 0 || buff_size < 1 || fxp->file_open != 2)
    return -FXPEINVAL;

  while(num_sent < buff_size) {
    if((fxpret= fxp_data_send(fxp, buff + num_sent, buff_size)) != FXPOK)
      break;
    num_sent += buff_size;
  }

  if(fxpret != FXPOK)
    if(fxp_stor_close(fxp) != FXPOK)
      error2("fxp_stor_close() failed, ignoring\n");

  if(fxpret != FXPOK && fxpret != -FXPETIMEOUT)
    return fxpret;

  debug("fxp_stor_write() succeeded - wrote [%u] bytes\n", buff_size);
  return FXPOK;
}
