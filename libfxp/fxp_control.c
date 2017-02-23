
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <millweed/net.h>
#include "fxp_private.h"

#define QUICK_OVERRIDE_TIMEOUT  500  //TODO: decreasing this improves performance

fxp_error_t fxp_ctrl_send_message(fxp_handle_t fxp, const char *format, ...)
{
  int ret;
  va_list args;
  char *msg, *next_msg;

  if(fxp == NULL || format == NULL)
    return -FXPENULLP;
  if(fxp->ctrl_sock == 0)
    return -FXPEINVAL;

  if(fxp->data_failed_timing_out == 1) {
    unsigned code;

    if(fxp_ctrl_receive_code_quick(fxp, &code, NULL) == -FXPETIMEOUT) {
      error2("server is currently timing out on a failed data connection establishment"
	     "and fxp_ctrl_receive_code_quick() returned -FXPETIMEOUT, returning\n");
      return -FXPETIMEOUT;
    }
    if(code == 421) {
      error2("fxp_ctrl_send_message() 421, disconnecting and returning\n");
      if(fxp_disconnect_hard(fxp) != FXPOK)
	error2("fxp_disconnect_hard() failed, ignoring\n");
      return -FXPECLOSED;
    }
    if(code != 425)
      error2("server recovered from a failed data connection establishment, but the returned "
	     "code was not 425 [%u], ignoring\n", code);
    fxp->data_failed_timing_out= 0;
  }

  va_start(args, format);
  vsnprintf(fxp->buff, sizeof(fxp->buff), format, args);
  va_end(args);

  for(next_msg= fxp->buff; next_msg != NULL && *next_msg != 0x0; ) {
    unsigned len;

    msg= next_msg;
    if((next_msg= strstr(msg, FXPEOL)) != NULL)
      next_msg += 2;

    len= (next_msg != NULL)? next_msg-msg: strlen(msg);
    if((ret= send(fxp->ctrl_sock, msg, len, 0)) == -1 || ret < len) {
      if(errno == EPIPE) {
	error2("send() failed, remote connection closed, disconnecting and returning - [%s]\n",
	       strerror(errno));
	fxp_disconnect_hard(fxp);  //server closed connection, don't need to be graceful
      }
      error2("send() failed, returning - [%s]\n", strerror(errno));
      return -FXPEUNKNOWN;
    }
  }

  if(fxp->ctrl_send_hook != NULL)
    (*fxp->ctrl_send_hook)(fxp, fxp->buff, ret);

  debug("fxp_ctrl_send_message() succeeded\n");
  return FXPOK;
}

fxp_error_t fxp_ctrl_receive(fxp_handle_t fxp, char *out_buff, unsigned *out_buff_size)
{
  int ret;
  fd_set fds;
  struct timeval tv= { 0, fxp->ctrl_recv_timeo * 1000 }, *ptv= NULL;

  if(fxp == NULL || out_buff == NULL)
    return -FXPENULLP;
  if(fxp->ctrl_sock == 0 || *out_buff_size < 1)
    return -FXPEINVAL;

  FD_ZERO(&fds);
  FD_SET(fxp->ctrl_sock, &fds);
  if(fxp->ctrl_recv_timeo > 0)
    ptv= &tv;

  if(select(fxp->ctrl_sock+1, &fds, NULL, NULL, ptv) != 1) {
    debug("select() timed out, returning - [%s]\n", strerror(errno));
    return -FXPETIMEOUT;
  }
  if((ret= recv(fxp->ctrl_sock, out_buff, *out_buff_size-1, 0)) < 1) {
    if(errno == EAGAIN || errno == EINPROGRESS) {
      debug("recv() timed out, returning - [%s]\n", strerror(errno));
      return -FXPETIMEOUT;
    }
    debug("recv() failed, returning - [%s]\n", strerror(errno));
    return -FXPEUNKNOWN;
  }
  out_buff[ret]= 0x0;
  *out_buff_size= ret;

  if(fxp->ctrl_recv_hook != NULL)
    (*fxp->ctrl_recv_hook)(fxp, out_buff, *out_buff_size);

  debug("fxp_ctrl_receive() succeeded - recieved [%d] bytes\n", ret);
  return FXPOK;
}

fxp_error_t fxp_ctrl_receive_code(fxp_handle_t fxp, unsigned *out_code, char **out_rest)
{
  unsigned codes_size= 1;
  return fxp_ctrl_receive_codes(fxp, out_code, &codes_size, out_rest);
}

fxp_error_t fxp_ctrl_receive_codes(fxp_handle_t fxp, unsigned *out_codes, unsigned *out_codes_size,
				   char **out_rest)
{
  char *ptr, *lines[10];
  int ptr_size, lines_size, cur_code= 0;
  const unsigned lines_orig_size= sizeof(lines)/sizeof(char *);
  fxp_error_t fxpret;

  if(fxp == NULL || out_codes == NULL || out_codes_size == NULL)
    return -FXPENULLP;
  if(*out_codes_size < 1)
    return -FXPEINVAL;

  ptr= fxp->buff;
  ptr_size= sizeof(fxp->buff);
  while(1) {
    char *frag= NULL;
    int parse_failed= 0;

    if((fxpret= fxp_ctrl_receive(fxp, ptr, &ptr_size)) != FXPOK && ptr == fxp->buff)
      break;
    ptr= fxp->buff;

    while(1) {
      int i;

      frag= NULL;
      lines_size= lines_orig_size;
      if(buff_to_lines(ptr, lines, &lines_size, &frag) != FXPOK || lines_size < 1) {
	parse_failed= fxpret != FXPOK;
	break;
      }

      for(i=0; i<lines_size; i++) {
	char *rest= NULL;
	unsigned tmp;

	if(line_to_code(lines[i], &tmp, (const char **)&rest) == FXPOK && tmp > 0) {
	  out_codes[cur_code++]= tmp;
	  if(cur_code == *out_codes_size) {
	    if(out_rest != NULL)
	      *out_rest= rest;
	    return FXPOK;
	  }
	}
      }
      if(!(lines_size >= lines_orig_size && (frag != NULL && *frag != 0x0)))  //done?
	break;

      ptr= frag;  //nope, still have a fragment to parse
    }

    if(cur_code > 0 || parse_failed)
      break;

    ptr= fxp->buff;
    ptr_size= sizeof(fxp->buff);
    if(frag != NULL && *frag != 0x0) {  //prepend the possible frag
      ptr_size -= strlen(frag);
      if(ptr != frag)
	strcpy(ptr, frag);
      ptr += strlen(ptr);
    }
  }

  if(fxpret != FXPOK)
    return fxpret;
  if(cur_code < 1)
    return -FXPEUNKNOWN;

  *out_codes_size= cur_code;
  debug("fxp_ctrl_receive_codes() succeeded\n");
  return FXPOK;
}

fxp_error_t fxp_ctrl_receive_code_quick(fxp_handle_t fxp, unsigned *out_code, char **out_rest)
{
  fxp_error_t fxpret;
  unsigned old_timeo;

  if(fxp == NULL || out_code == NULL)
    return -FXPENULLP;

  if(fxp->ctrl_recv_timeo < QUICK_OVERRIDE_TIMEOUT)
    return fxp_ctrl_receive_code(fxp, out_code, out_rest);

  old_timeo= fxp->ctrl_recv_timeo;
  fxp->ctrl_recv_timeo= QUICK_OVERRIDE_TIMEOUT;
  fxpret= fxp_ctrl_receive_code(fxp, out_code, out_rest);
  fxp->ctrl_recv_timeo= old_timeo;

  return fxpret;
}

fxp_error_t fxp_connect(fxp_handle_t fxp, unsigned long timeout)
{
  fxp_error_t fxpret;
  int sock, oldargs;
  struct sockaddr_in addr;
  struct hostent *ent;
  unsigned code;

  if(fxp == NULL)
    return -FXPENULLP;

  if(fxp->ctrl_sock != 0) {
    debug("control connection still open, implicitly calling fxp_disconnect()\n");
    if(fxp_disconnect(fxp) < 0)
      error2("fxp_disconnect() failed, ignoring\n");
  }
  if(fxp->ctrl_sock != 0)
    return -FXPEUNKNOWN;

  if((sock= socket(PF_INET, SOCK_STREAM, 0)) == -1) {
    error2("socket() failed, returning - [%s]\n", strerror(errno));
    return -FXPEUNKNOWN;
  }

  memset(&addr, 0, sizeof(addr));
  addr.sin_family= AF_INET;
  addr.sin_port= htons(fxp->url.port);
  if((ent= gethostbyname(fxp->url.host)) == NULL) {
    error2("gethostbyname() failed, returning - [%s]\n", strerror(errno));
    return -FXPENOTFOUND;
  }
  addr.sin_addr= *(struct in_addr *)ent->h_addr_list[0];

  if((oldargs= fcntl(sock, F_GETFL)) == -1) {
    error2("fcntl(F_GETFL) failed, returning - [%s]\n", strerror(errno));
    close(sock);
    return -FXPEUNSUPP;
  }
  if(!(oldargs & O_NONBLOCK) && fcntl(sock, F_SETFL, oldargs | O_NONBLOCK) != 0) {
    error2("fcntl(F_SETFL, O_NONBLOCK) failed, returning - [%s]\n", strerror(errno));
    close(sock);
    return -FXPEUNSUPP;
  }

  debug("calling connect_with_timeout(%s, %u, %lu)\n", fxp->url.host, fxp->url.port, timeout);
  if(connect_with_timeout(sock, (struct sockaddr *)&addr, sizeof(addr), timeout) < 0) {
    fxpret= (errno == EAGAIN || errno == EINPROGRESS)? -FXPETIMEOUT: -FXPEUNKNOWN;
    error2("connect_with_timeout() failed, returning - [%s]\n", strerror(errno));
    close(sock);
    return fxpret;
  }

  fxp->ctrl_sock= sock;

  if((fxpret= fxp_ctrl_receive_code(fxp, &code, NULL)) != FXPOK) {
    error2("fxp_ctrl_receive_code() failed, calling fxp_disconnect_hard() and returning\n");
    fxp_disconnect_hard(fxp);
    return fxpret;
  }
  if(code == 421) {
    error2("connect 421, disconnecting and returning\n");
    if(fxp_disconnect_hard(fxp) != FXPOK)
      error2("fxp_disconnect_hard() failed, ignoring\n");
    return -FXPECLOSED;
  }
  if(code != 220)
    error2("connect unknown code [%u], ignoring\n", code);

  debug("fxp_connect() succeeded\n");
  return FXPOK;
}

fxp_error_t fxp_disconnect_hard(fxp_handle_t fxp)
{
  if(fxp == NULL)
    return -FXPENULLP;

  if(fxp_data_cleanup(fxp) != FXPOK)  //TODO: should this be _data_cleanup_hard()?
    error2("fxp_data_cleanup() failed, ignoring\n");

  if(fxp->ctrl_sock != 0) {
    if(close(fxp->ctrl_sock) != 0)
      error2("close() failed, ignoring - [%s]\n", strerror(errno));
    fxp->ctrl_sock= 0;
  }

  //TODO: anything else (vars) need cleaning up?

  debug("fxp_disconnect_hard() succeeded\n");
  return FXPOK;
}

fxp_error_t fxp_disconnect(fxp_handle_t fxp)
{
  unsigned code;

  if(fxp == NULL)
    return -FXPENULLP;

  if(fxp->ctrl_sock != 0) {
    if(fxp_ctrl_send_message(fxp, "QUIT" FXPEOL) != FXPOK)
      error2("fxp_ctrl_send_message(QUIT) failed, ignoring\n");
    else {
      if(fxp_ctrl_receive_code(fxp, &code, NULL) != FXPOK)
	error2("fxp_ctrl_receive_code() failed, ignoring\n");
      else if(code == 421)
	error2("QUIT 421, ignoring\n");
      else if(code != 221)
	error2("QUIT unknown code [%u], ignoring\n", code);
      else
	debug("QUIT succeeded\n");
    }
  }

  if(fxp_disconnect_hard(fxp) != FXPOK)
    error2("fxp_disconnect_hard() failed, ignoring\n");

  debug("fxp_disconnect() succeeded\n");
  return FXPOK;
}

fxp_error_t fxp_login(fxp_handle_t fxp, const char *username, const char *password)
{
  char buff[2*SMBUFFSZ];
  unsigned code;
  fxp_error_t fxpret;

  if(fxp == NULL)
    return -FXPENULLP;
  if(fxp->ctrl_sock == 0)
    return -FXPEINVAL;

  sprintf(buff, "USER %s", (username != NULL)? username: fxp->url.user);
  if((fxpret= fxp_ctrl_send_message(fxp, "%s" FXPEOL, buff)) != FXPOK) {
    error2("fxp_ctrl_send_message(USER) failed, returning\n");
    return fxpret;
  }
  if((fxpret= fxp_ctrl_receive_code(fxp, &code, NULL)) != FXPOK) {
    error2("USER fxp_ctrl_receive_code() failed, returning\n");
    return fxpret;
  }
  if(code == 421) {
    error2("USER 421, disconnecting and returning\n");
    if(fxp_disconnect_hard(fxp) != FXPOK)
      error2("fxp_disconnect_hard() failed, ignoring\n");
    return -FXPECLOSED;
  }
  if(code == 350) {
    error2("USER failed, returning\n");
    return -FXPEUNKNOWN;
  }
  if(code == 420) {
    error2("USER failed (username not allowed), returning\n");
    return -FXPEACCESS;
  }
  if(code == 530) {
    error2("USER failed (too many users), returning\n");
    return -FXPEACCESS;
  }
  if(code != 331 && code != 230) {
    error2("USER unknown code [%u], returning\n", code);
    return -FXPEUNKNOWN;
  }
  debug("USER succeeded\n");

  if(code == 230) {
    debug("PASS not needed, skipping\n");
    goto fxp_login_after_pass;
  }

  sprintf(buff, "PASS %s", (password != NULL)? password: fxp->url.pass);
  if((fxpret= fxp_ctrl_send_message(fxp, "%s" FXPEOL, buff)) != FXPOK) {
    error2("fxp_ctrl_send_message(PASS) failed, returning\n");
    return fxpret;
  }
  if((fxpret= fxp_ctrl_receive_code(fxp, &code, NULL)) != FXPOK) {
    error2("PASS fxp_ctrl_receive_code() failed, returning\n");
    return fxpret;
  }
  if(code == 530) {
    error2("PASS failed, returning\n");
    return -FXPEACCESS;
  }
  if(code != 230) {
    error2("PASS unknown code [%u], returning\n", code);
    return -FXPEUNKNOWN;
  }
  debug("PASS succeeded\n");

  if(password != NULL)
    strncpy(fxp->url.pass, password, sizeof(fxp->url.pass));

 fxp_login_after_pass:
  if(username != NULL)  //don't save username until after successful login
    strncpy(fxp->url.user, username, sizeof(fxp->url.user));

  debug("fxp_login() succeeded\n");
  return FXPOK;
}

fxp_error_t fxp_noop(fxp_handle_t fxp)
{
  unsigned code;
  fxp_error_t fxpret;

  if(fxp == NULL)
    return -FXPENULLP;
  if(fxp->ctrl_sock == 0)
    return -FXPEINVAL;

  if((fxpret= fxp_ctrl_send_message(fxp, "NOOP" FXPEOL)) != FXPOK) {
    error2("fxp_ctrl_send_message(NOOP) failed, returning\n");
    return fxpret;
  }
  if((fxpret= fxp_ctrl_receive_code(fxp, &code, NULL)) != FXPOK) {
    error2("fxp_ctrl_receive_code() failed, returning\n");
    return fxpret;
  }
  if(code == 500) {
    error2("NOOP failed, returning\n");
    return -FXPEUNKNOWN;
  }
  if(code == 421) {
    error2("NOOP 421, disconnecting and returning\n");
    if(fxp_disconnect_hard(fxp) != FXPOK)
      error2("fxp_disconnect_hard() failed, ignoring\n");
    return -FXPECLOSED;
  }
  if(code != 200) {
    error2("NOOP unknown code [%u], returning\n", code);
    return -FXPEUNKNOWN;
  }
  debug("NOOP succeeded\n");

  debug("fxp_noop() succeeded\n");
  return FXPOK;
}
