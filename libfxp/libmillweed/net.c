
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/time.h>

//millweed headers
#include <millweed/net.h>

int connect_with_timeout(int sockfd, struct sockaddr *serv_addr, size_t addrlen,
			 unsigned long timeout)
{
  return connect_with_timeout_watch(sockfd, 0, serv_addr, addrlen, timeout);
}

int connect_with_timeout_watch(int sockfd, int watchfd, struct sockaddr *serv_addr, size_t addrlen,
			       unsigned long timeout)
{
  int oldargs, ret, retcode, timedout= 0;

  if((oldargs= fcntl(sockfd, F_GETFL)) == -1) {
    error2("fcntl(F_GETFL) failed, returning\n");
    return -1;
  }
  if(oldargs & O_NONBLOCK)
    debug("socket already nonblocking, skipping fcntl(O_NONBLOCK)\n");
  else if(fcntl(sockfd, F_SETFL, oldargs | O_NONBLOCK) != 0) {
    error2("fcntl(O_NONBLOCK) failed, returning\n");
    return -1;
  }

  retcode= -1;
  if(connect(sockfd, serv_addr, addrlen) != 0 && errno != EINPROGRESS)
    error2("connect() failed, returning\n");
  else {  //good, now wait for timeout number of seconds
    fd_set rd_fds, wr_fds;
    struct timeval tv= { 0, timeout * 1000 }, *ptv= NULL;

    debug("connect() returned EINPROGRESS, calling select()\n");

    FD_ZERO(&wr_fds);
    FD_SET(sockfd, &wr_fds);
    if(watchfd > 0) {
      FD_ZERO(&rd_fds);
      FD_SET(watchfd, &rd_fds);
    }
    if(timeout > 0)
      ptv= &tv;

    if((ret= select(sockfd+1, (watchfd > 0)? &rd_fds: NULL, &wr_fds, NULL, ptv)) > 0) {
      if(FD_ISSET(sockfd, &wr_fds)) {
	int val;
	size_t vallen= sizeof(val);

	if(getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (void *)&val, &vallen) == 0 && val == 0) {
	  debug("connect() completed successfully after select()\n");
	  retcode= 0;
	}
	else
	  error2("connect() failed, returning\n");
      }
      else /* if(FD_ISSET(watchfd, &rd_fds) */ {
	error2("select() said incoming data on watchfd, returning\n");
	retcode= 1;
      }
    }
    else if(ret == 0) {
      error2("connect() timed out, returning\n");
      timedout= 1;
    }
    else
      error2("connect() failed, returning\n");
  }

  if(!(oldargs & O_NONBLOCK) && fcntl(sockfd, F_SETFL, oldargs) != 0) {
    error2("fcntl(~O_NONBLOCK) failed, returning\n");
    return -1;
  }

  if(timedout)
    errno= EAGAIN;

  return retcode;
}

int create_active_socket()
{
  int sock, oldargs, optval= 1;

  if((sock= socket(PF_INET, SOCK_STREAM, 0)) == -1)
    error2("socket() failed, returning - [%s]\n", strerror(errno));

  if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *)&optval, sizeof(optval)) == -1)
    error2("setsockopt(SO_REUSEADDR) failed, ignoring - [%s]\n", strerror(errno));

  if(listen(sock, 1) == -1)
    error2("listen() failed, returning - [%s]\n", strerror(errno));

  if((oldargs= fcntl(sock, F_GETFL)) == -1) {
    error2("fcntl(F_GETFL) failed, returning\n");
    return -1;
  }
  if(fcntl(sock, F_SETFL, oldargs | O_NONBLOCK) != 0) {
    error2("fcntl(O_NONBLOCK) failed, returning - [%s]\n", strerror(errno));
    return -1;
  }

  if(accept(sock, NULL, 0) == -1 && errno != EAGAIN) {
    error2("nonblocking accept() failed, returning - [%s]\n", strerror(errno));
    close(sock);
    return -1;
  }

  return sock;
}

int accept_with_timeout(int s, struct sockaddr *addr, socklen_t *addrlen,
			unsigned long timeout)
{
  int oldargs, retcode, ret;
  fd_set fds;
  struct timeval tv= {};

  if((oldargs= fcntl(s, F_GETFL)) == -1) {
    error2("fcntl(F_GETFL) failed, returning\n");
    return -1;
  }
  if(fcntl(s, F_SETFL, oldargs | O_NONBLOCK) != 0) {
    error2("fcntl(O_NONBLOCK) failed, returning - [%s]\n", strerror(errno));
    return -1;
  }

  FD_ZERO(&fds);
  FD_SET(s, &fds);
  tv.tv_usec= timeout * 1000;
  retcode= -1;
  if((ret= select(s+1, &fds, NULL, NULL, &tv)) > 0 && FD_ISSET(s, &fds)) {
    if((ret= accept(s, addr, addrlen)) == -1)
      error2("accept() failed, returning\n");
    else {
      debug("accept() completed successfully after select()\n");
      retcode= ret;
    }
  }
  else if(ret == 0) {
    error2("accept() timed out, returning\n");
    errno= EAGAIN;
  }
  else
    error2("accept() failed, returning\n");

  return retcode;
}
