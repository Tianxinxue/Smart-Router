#ifndef _UDP_H
#define _UDP_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int udp_server_init(int port);
int tcp_server_init(int port);

#endif  /*_UDP_H*/
