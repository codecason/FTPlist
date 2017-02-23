
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <millweed/net.h>
#include "fxp_private.h"

fxp_error_t fxp_data_cleanup(fxp_handle_t fxp)
{
  if(fxp == NULL)
    return -FXPENULLP;

  if(fxp->data_actv_listen_sock != 0) {
    if(close(fxp->data_actv_listen_sock) != 0)
      error2("close() failed, ignoring - [%s]\n", strerror(errno));
    fxp->data_actv_listen_sock= 0;
  }

  if(fxp->data_sock != 0) {
    if(close(fxp->data_sock) != 0)
      error2("close() failed, ignoring - [%s]\n", strerror(errno));
    fxp->data_sock= 0;
  }

  free(fxp->data_pasv_host);
  fxp->data_pasv_host= NULL;
  fxp->data_conn_setup= 0;
  fxp->data_port= 0;
  fxp->file_open= 0;
  fxp->last_xfer_code= 0;

  debug("fxp_data_cleanup() succeeded\n");
  return FXPOK;
}

fxp_error_t fxp_establish_mode_passive_setup(fxp_handle_t fxp)
{
  char *ptr, *tok, *rest= NULL;
  unsigned host[4], port[2], code;
  fxp_error_t fxpret;

  if(fxp == NULL)
    return -FXPENULLP;
  if(fxp->ctrl_sock == 0 || fxp->data_conn_setup != 0)
    return -FXPEINVAL;

  if((fxpret= fxp_ctrl_send_message(fxp, "PASV" FXPEOL)) != FXPOK) {
    error2("fxp_ctrl_send_message(PASV) failed, returning\n");
    return fxpret;
  }
  if((fxpret= fxp_ctrl_receive_code(fxp, &code, &rest)) != FXPOK) {
    error2("fxp_ctrl_receive_code() failed, returning\n");
    return fxpret;
  }
  if(code == 421) {
    error2("PASV 421, disconnecting and returning\n");
    if(fxp_disconnect_hard(fxp) != FXPOK)
      error2("fxp_disconnect_hard() failed, ignoring\n");
    return -FXPECLOSED;
  }
  if(code != 227) {
    error2("PASV unknown code [%u], returning\n", code);
    return -FXPEUNKNOWN;
  }
  debug("PASV succeeded\n");

  if(strtok_mw(rest, "(", &ptr) == NULL || (tok= strtok_mw(NULL, ")", &ptr)) == NULL) {
    error2("error parsing response, returning\n");
    return -FXPEINVAL;
  }
  if(sscanf(tok, "%u,%u,%u,%u,%u,%u", &host[0], &host[1], &host[2], &host[3],
	    &port[0], &port[1]) != 6) {
    error2("error parsing response, returning\n");
    return -FXPEINVAL;
  }

  snprintf(fxp->buff, sizeof(fxp->buff), "%u.%u.%u.%u", host[0], host[1], host[2], host[3]);
  fxp->data_pasv_host= strdup(fxp->buff);
  fxp->data_port= port[0] << 8 | port[1];  //high byte came first

  debug("fxp_establish_mode_passive_setup() succeeded\n");
  return FXPOK;
}

fxp_error_t fxp_establish_mode_passive_establish(fxp_handle_t fxp)
{
  int sock, ret;
  struct sockaddr_in addr;
  struct hostent *ent;

  if(fxp == NULL || fxp->data_pasv_host == NULL)
    return -FXPENULLP;
  if(fxp->ctrl_sock == 0 || fxp->data_conn_setup == 0 || *fxp->data_pasv_host == 0x0 ||
     fxp->data_port == 0)
    return -FXPEINVAL;

  memset(&addr, 0, sizeof(addr));
  addr.sin_family= AF_INET;
  addr.sin_port= htons(fxp->data_port);
  if((ent= gethostbyname(fxp->data_pasv_host)) == NULL) {
    error2("gethostbyname() failed, returning\n");
    return -FXPENOTFOUND;
  }
  addr.sin_addr= *(struct in_addr *)ent->h_addr_list[0];

  if((sock= socket(PF_INET, SOCK_STREAM, 0)) == -1) {
    error2("socket() failed, returning - [%s]\n", strerror(errno));
    return -FXPEUNKNOWN;
  }

  debug("attempting passive data connection to %s:%u\n", fxp->data_pasv_host, fxp->data_port);
  if((ret= connect_with_timeout_watch(sock, fxp->ctrl_sock, (struct sockaddr *)&addr, sizeof(addr),
				      fxp->data_estab_timeo)) < 0) {
    fxp_error_t fxpret= (errno == EAGAIN || errno == EINPROGRESS)? -FXPETIMEOUT: -FXPEUNKNOWN;
    error2("connect_with_timeout_watch() failed, returning\n");
    close(sock);
    fxp->data_failed_timing_out= 1;
    return fxpret;
  }
  else if(ret == 1) {
    error2("connect_with_timeout_watch() interrupted by incoming data\n");
    close(sock);
    return -FXPEINTERRUPT;
  }

  fxp->data_sock= sock;

  debug("fxp_establish_mode_passive_establish() succeeded\n");
  return FXPOK;
}

fxp_error_t fxp_establish_mode_active_setup(fxp_handle_t fxp)
{
  int sock;
  struct sockaddr_in addr;
  size_t addrlen;
  unsigned short port;
  unsigned char myip[SMBUFFSZ], *tmp;
  unsigned code;
  fxp_error_t fxpret;

  if(fxp == NULL)
    return -FXPENULLP;
  if(fxp->ctrl_sock == 0 || fxp->data_conn_setup != 0)
    return -FXPEINVAL;

  addrlen= sizeof(addr);
  if(getsockname(fxp->ctrl_sock, (struct sockaddr *)&addr, &addrlen) == -1) {  //get local ip
    error2("getsockname(ctrl_sock) failed, returning\n");
    return -FXPENOTFOUND;
  }
  strncpy(myip, inet_ntoa(addr.sin_addr), sizeof(myip));

  if((sock= create_active_socket()) < 1) {
    error2("create_active_socket() failed, returning\n");
    return -FXPEUNKNOWN;
  }

  addrlen= sizeof(addr);
  if(getsockname(sock, (struct sockaddr *)&addr, &addrlen) == -1) {  //get local port
    error2("getsockname(sock) failed, returning\n");
    close(sock);
    return -FXPENOTFOUND;
  }
  port= ntohs(addr.sin_port);

  for(tmp= myip; *tmp != 0x0; tmp++)
    if(*tmp == '.')
      *tmp= ',';
  if((fxpret= fxp_ctrl_send_message(fxp, "PORT %s,%u,%u" FXPEOL,
				    myip, port >> 8 & 0xff, port & 0xff)) != FXPOK) {
    error2("fxp_ctrl_send_message(PORT) failed, returning\n");
    close(sock);
    return fxpret;
  }
  if((fxpret= fxp_ctrl_receive_code(fxp, &code, NULL)) != FXPOK) {
    error2("fxp_ctrl_receive_code() failed, returning\n");
    close(sock);
    return fxpret;
  }
  if(code == 421) {
    error2("PORT 421, disconnecting and returning\n");
    if(fxp_disconnect_hard(fxp) != FXPOK)
      error2("fxp_disconnect_hard() failed, ignoring\n");
    return -FXPECLOSED;
  }
  if(code == 500) {
    error2("PORT syntax error, returning\n");
    close(sock);
    return -FXPEINVAL;
  }
  if(code != 200) {
    error2("PORT failed, returning\n");
    close(sock);
    return -FXPEUNKNOWN;
  }
  debug("PORT succeeded\n");

  fxp->data_actv_listen_sock= sock;  //connection will be established later
  fxp->data_port= port;

  debug("fxp_establish_mode_active_setup() succeeded\n");
  return FXPOK;
}

fxp_error_t fxp_establish_mode_active_establish(fxp_handle_t fxp)
{
  int possible_client;

  if(fxp == NULL)
    return -FXPENULLP;
  if(fxp->ctrl_sock == 0 || fxp->data_conn_setup == 0 || fxp->data_port == 0 ||
     fxp->data_actv_listen_sock == 0)
    return -FXPEINVAL;

  possible_client= accept_with_timeout(fxp->data_actv_listen_sock, NULL, 0, fxp->data_estab_timeo);
  if(possible_client < 1) {
    error2("accept_with_timeout() timed out, returning\n");
    fxp->data_failed_timing_out= 1;
    return -FXPETIMEOUT;
  }

  if(close(fxp->data_actv_listen_sock) < 0)
    error2("close() failed, ignoring - [%s]\n", strerror(errno));
  fxp->data_actv_listen_sock= 0;

  fxp->data_sock= possible_client;

  debug("fxp_establish_mode_active_establish() succeeded\n");
  return FXPOK;
}

fxp_error_t fxp_data_establish_setup(fxp_handle_t fxp)
{
  fxp_error_t fxpret;

  if(fxp == NULL)
    return -FXPENULLP;
  if(fxp->ctrl_sock == 0 || fxp->data_conn_setup != 0)
    return -FXPEINVAL;

  if(fxp->data_mode == -1) {
    if((fxpret= fxp_mode(fxp, FXP_MODE_PASSIVE)) != FXPOK) {  //default to passive
      error2("fxp_mode(FXP_MODE_PASSIVE) failed, returning\n");
      return fxpret;
    }
  }

  if(fxp_data_cleanup(fxp) != FXPOK)  //implicit cleanup
    error2("fxp_data_cleanup() failed, ignoring\n");

  switch(fxp->data_mode) {
  case FXP_MODE_ACTIVE:
    if((fxpret= fxp_establish_mode_active_setup(fxp)) != FXPOK) {
      error2("fxp_establish_mode_active_setup() failed, returning\n");
      return fxpret;
    }
    break;
  case FXP_MODE_PASSIVE:
    if((fxpret= fxp_establish_mode_passive_setup(fxp)) != FXPOK) {
      error2("fxp_establish_mode_passive_setup() failed, returning\n");
      return fxpret;
    }
    break;
  default:
    error2("unknown configured mode [%d], returning\n", fxp->data_mode);
    return -FXPEINVAL;
  }

  fxp->data_conn_setup= 1;

  debug("fxp_data_establish_setup() succeeded\n");
  return FXPOK;
}

fxp_error_t fxp_data_establish(fxp_handle_t fxp)
{
  fxp_error_t fxpret;

  if(fxp == NULL)
    return -FXPENULLP;
  if(fxp->ctrl_sock == 0)
    return -FXPEINVAL;

  if(fxp->data_conn_setup == 0) {
    if((fxpret= fxp_data_establish_setup(fxp)) != FXPOK) {  //implicit setup call
      error2("fxp_data_establish_setup() failed, returning\n");
      return fxpret;
    }
  }
  if(fxp->data_conn_setup == 0)
    return -FXPEINVAL;

  switch(fxp->data_mode) {
  case FXP_MODE_ACTIVE:
    if((fxpret= fxp_establish_mode_active_establish(fxp)) != FXPOK) {
      error2("fxp_establish_mode_active_establish() failed, returning\n");
      return fxpret;
    }
    break;
  case FXP_MODE_PASSIVE:
    if((fxpret= fxp_establish_mode_passive_establish(fxp)) != FXPOK) {
      error2("fxp_establish_mode_passive_establish() failed, returning\n");
      return fxpret;
    }
    break;
  default:
    error2("unknown configured mode [%d], returning\n", fxp->data_mode);
    return -FXPEINVAL;
  }

  fxp->data_conn_setup= 0;

  debug("fxp_data_establish() succeeded\n");
  return FXPOK;
}

fxp_error_t fxp_data_receive(fxp_handle_t fxp, char *out_buff, unsigned *out_buff_size)
{
  fd_set fds;
  struct timeval tv= {};
  fxp_error_t fxpret;
  int ret;

  if(fxp == NULL || out_buff == NULL || out_buff_size == NULL)
    return -FXPENULLP;
  if(fxp->ctrl_sock == 0 || *out_buff_size < 1)
    return -FXPEINVAL;

  if(fxp->data_conn_setup != 0) {
    if((fxpret= fxp_data_establish(fxp)) != FXPOK) {
      error2("fxp_data_establish() failed, returning\n");
      return fxpret;
    }
  }
  if(fxp->data_sock == 0)
    return -FXPEINVAL;

  if(fxp->data_recv_timeo > 0) {

    FD_ZERO(&fds);
    FD_SET(fxp->data_sock, &fds);
    tv.tv_usec= fxp->data_recv_timeo * 1000;
    if(select(fxp->data_sock+1, &fds, NULL, NULL, &tv) != 1) {
      debug("select() timed out, returning - [%s]\n", strerror(errno));
      return -FXPETIMEOUT;
    }
  }

  if((ret= recv(fxp->data_sock, out_buff, *out_buff_size, 0)) < 1) {
    if(errno == EAGAIN || errno == EINPROGRESS) {
      debug("recv() timed out, returning - [%s]\n", strerror(errno));
      return -FXPETIMEOUT;
    }
    if(errno == EPIPE || errno == ENOTSOCK) {
      debug("recv() connection closed, cleaning up and returning - [%s]\n", strerror(errno));
      if(fxp_data_cleanup(fxp) != FXPOK)
	error2("fxp_data_cleanup() failed, ignoring\n");
      return -FXPEINVAL;
    }
    debug("recv() failed, returning - [%s]\n", strerror(errno));
    return -FXPEUNKNOWN;
  }
  *out_buff_size= ret;

  debug("fxp_data_receive() succeeded - received [%d] bytes\n", ret);
  return FXPOK;
}

fxp_error_t fxp_data_send(fxp_handle_t fxp, const char *buff, unsigned buff_size)
{
  fd_set fds;
  struct timeval tv= {};
  fxp_error_t fxpret;
  int ret;

  if(fxp == NULL || buff == NULL)
    return -FXPENULLP;
  if(fxp->ctrl_sock == 0 || buff_size < 1)
    return -FXPEINVAL;

  if(fxp->data_conn_setup != 0) {
    if((fxpret= fxp_data_establish(fxp)) != FXPOK) {
      error2("fxp_data_establish() failed, returning\n");
      return fxpret;
    }
  }
  if(fxp->data_sock == 0)
    return -FXPEINVAL;

  if(fxp->data_send_timeo > 0) {
    FD_ZERO(&fds);
    FD_SET(fxp->data_sock, &fds);
    tv.tv_usec= fxp->data_send_timeo * 1000;
    if(select(fxp->data_sock+1, NULL, &fds, NULL, &tv) != 1) {
      debug("select() timed out, returning - [%s]\n", strerror(errno));
      return -FXPETIMEOUT;
    }
  }

  if((ret= send(fxp->data_sock, buff, buff_size, 0)) < 1) {
    if(errno == EAGAIN || errno == EINPROGRESS || errno == EWOULDBLOCK) {
      debug("send() timed out, returning - [%s]\n", strerror(errno));
      return -FXPETIMEOUT;
    }
    if(errno == EPIPE || errno == ENOTSOCK) {
      debug("send() connection closed, returning - [%s]\n", strerror(errno));
      if(fxp_data_cleanup(fxp) != FXPOK)
	error2("fxp_data_cleanup() failed, ignoring\n");
      return -FXPEINVAL;
    }
    debug("send() failed, returning - [%s]\n", strerror(errno));
    return -FXPEUNKNOWN;
  }

  debug("fxp_data_send() succeeded - sent [%d] bytes\n", ret);
  return FXPOK;
}
