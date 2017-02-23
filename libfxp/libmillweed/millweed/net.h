
#ifndef LIBMILLWEED_NET_H
#define LIBMILLWEED_NET_H


#include <millweed/misc.h>

#ifdef __cplusplus
extern "C" {
#endif

int connect_with_timeout(int sockfd, struct sockaddr *serv_addr, size_t addrlen, unsigned long timeout);
//like connect_with_timeout() but returns 1 if data was incoming on watchfd
int connect_with_timeout_watch(int sockfd, int watchfd, struct sockaddr *serv_addr, size_t addrlen,
			       unsigned long timeout);

//creates a nonblocking socket, and calls listen() and accept()
int create_active_socket();
//calls accept() on the socket with a timeout
int accept_with_timeout(int s, struct sockaddr *addr, size_t *addrlen, unsigned long timeout);

#ifdef __cplusplus
}
#endif


#endif
