#ifndef SERVER_UTILS_H
#define SERVER_UTILS_H

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>

#include "telemetry.h"

#define PORT 8080
#define BACKLOG_SIZE 10

int socket_create();
void set_address(struct sockaddr_in *addr);
void bind_socket(int listen_fd, struct sockaddr_in *addr);
void listen_socket(int listen_fd);
void client_error(nfds_t *nfds, nfds_t *current_nfds , nfds_t *i, struct pollfd **fds);
int fds_realloc(struct pollfd **fds_ptr, size_t *fds_capacity_ptr);
ssize_t send_all(int sockfd, const unsigned char *buf, size_t len);
#endif // SERVER_UTILS_H