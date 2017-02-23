
#include <stdio.h>

#include "fxp_private.h"

fxp_error_t fxp_set_control_send_hook(fxp_handle_t fxp, fxp_send_hook hook)
{
  if(fxp == NULL || hook == NULL)
    return -FXPENULLP;

  if(fxp->ctrl_send_hook != NULL)
    mess2("replacing old fxp->ctrl_send_hook with the new hook\n");

  fxp->ctrl_send_hook= hook;

  debug("fxp_set_control_send_hook() succeeded\n");
  return FXPOK;
}

fxp_error_t fxp_set_control_receive_hook(fxp_handle_t fxp, fxp_receive_hook hook)
{
  if(fxp == NULL || hook == NULL)
    return -FXPENULLP;

  if(fxp->ctrl_recv_hook != NULL)
    mess2("replacing old fxp->ctrl_recv_hook with the new hook\n");

  fxp->ctrl_recv_hook= hook;

  debug("fxp_set_control_receive_hook() succeeded\n");
  return FXPOK;
}

fxp_error_t fxp_set_control_receive_timeout(fxp_handle_t fxp, unsigned long timeout)
{
  if(fxp == NULL)
    return -FXPENULLP;

  fxp->ctrl_recv_timeo= timeout;

  debug("fxp_set_control_receive_timeout() succeeded\n");
  return FXPOK;
}

fxp_error_t fxp_set_data_establish_timeout(fxp_handle_t fxp, unsigned long timeout)
{
  if(fxp == NULL)
    return -FXPENULLP;

  fxp->data_estab_timeo= timeout;

  debug("fxp_set_data_establish_timeout() succeeded\n");
  return FXPOK;
}

fxp_error_t fxp_set_data_receive_timeout(fxp_handle_t fxp, unsigned long timeout)
{
  if(fxp == NULL)
    return -FXPENULLP;

  fxp->data_recv_timeo= timeout;

  debug("fxp_set_data_receive_timeout() succeeded\n");
  return FXPOK;
}

fxp_error_t fxp_set_data_send_timeout(fxp_handle_t fxp, unsigned long timeout)
{
  if(fxp == NULL)
    return -FXPENULLP;

  fxp->data_send_timeo= timeout;

  debug("fxp_set_data_send_timeout() succeeded\n");
  return FXPOK;
}

fxp_error_t fxp_type(fxp_handle_t fxp, fxp_type_t type)
{
  unsigned code;
  fxp_error_t fxpret;

  if(fxp == NULL)
    return -FXPENULLP;
  if(fxp->ctrl_sock == 0)
    return -FXPEINVAL;

  if(fxp->data_type == type)
    return -FXPENOCHANGE;

  switch(type) {
  case FXP_TYPE_ASCII:
    fxpret= fxp_ctrl_send_message(fxp, "TYPE A" FXPEOL);
    break;
  case FXP_TYPE_BINARY:
    fxpret= fxp_ctrl_send_message(fxp, "TYPE I" FXPEOL);
    break;
  default:
    return -FXPEINVAL;
  }
  if(fxpret != FXPOK) {
    error2("fxp_ctrl_send_message(TYPE) failed, returning\n");
    return fxpret;
  }
  if((fxpret= fxp_ctrl_receive_code(fxp, &code, NULL)) != FXPOK) {
    error2("fxp_ctrl_receive_code() failed, returning\n");
    return fxpret;
  }
  if(code == 421) {
    error2("TYPE 421, disconnecting and returning\n");
    if(fxp_disconnect_hard(fxp) != FXPOK)
      error2("fxp_disconnect_hard() failed, ignoring\n");
    return -FXPECLOSED;
  }
  if(code == 500) {
    error2("TYPE syntax error, returning\n");
    return -FXPEINVAL;
  }
  if(code != 200) {
    error2("TYPE unknown code [%u], returning\n", code);
    return -FXPEUNKNOWN;
  }
  debug("TYPE succeeded\n");

  fxp->data_type= type;

  debug("fxp_type() succeeded\n");
  return FXPOK;
}

fxp_error_t fxp_mode(fxp_handle_t fxp, fxp_mode_t mode)
{
  if(fxp == NULL)
    return -FXPENULLP;
  if(fxp->data_conn_setup != 0)  //shouldn't be in mid-data-connect!
    return -FXPEINVAL;

  if(fxp->data_mode == mode)
    return -FXPENOCHANGE;

  fxp->data_port= 0;  //this is only set when client switches to active mode

  switch(mode) {
  case FXP_MODE_ACTIVE:
    fxp->data_mode= FXP_MODE_ACTIVE;
    break;
  case FXP_MODE_PASSIVE:
    fxp->data_mode= FXP_MODE_PASSIVE;
    break;
  default:
    error2("unknown mode [%d], returning\n", mode);
    return -FXPEINVAL;
  }

  debug("fxp_mode() succeeded\n");
  return FXPOK;
}
