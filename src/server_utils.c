#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>    
#include <errno.h>  

#include "server_utils.h"


int socket_create() {
    int listen_fd;
    int optval = 1;

    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        perror("setsockopt");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    return listen_fd;
}

void set_address(struct sockaddr_in *addr) {
    memset(addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = INADDR_ANY;
    addr->sin_port = htons(PORT);
}

void bind_socket(int listen_fd, struct sockaddr_in *addr) {
    if (bind(listen_fd, (struct sockaddr *)addr, sizeof(*addr)) < 0) {
        perror("bind");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }
}

void listen_socket(int listen_fd) {
    if (listen(listen_fd, BACKLOG_SIZE) < 0) {
        perror("listen");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }
} 
 
int fds_realloc(struct pollfd **fds_ptr, size_t *fds_capacity_ptr) {
    int old_capacity = *fds_capacity_ptr;
    int new_capacity = old_capacity * 2;

    struct pollfd *temp_fds = realloc(*fds_ptr, sizeof(struct pollfd) * new_capacity);

    if (temp_fds == NULL) {
        perror("realloc");
        return -1;
    } else {
        *fds_ptr = temp_fds;
        *fds_capacity_ptr = new_capacity;
        return 0;
    }
}

void client_error(nfds_t *nfds, nfds_t *i, struct pollfd **fds) {

    close((*fds)[*i].fd);
    
    (*fds)[*i] = (*fds)[*nfds - 1];
    (*nfds) --;
    (*i)--;
}

ssize_t send_all(int sockfd, const unsigned char *buf, size_t len) {
    size_t total_sent = 0;
    ssize_t bytes_sent;

    if (buf == NULL || len == 0) {
        return 0;
    }

    while (total_sent < len) {
        bytes_sent = send(sockfd, buf + total_sent, len - total_sent, 0);
        if (bytes_sent == -1) {
            if (errno == EINTR) {
                perror("send EINTR");
                continue; // Retry if interrupted by a signal
            } else {
                // Handle other errors
                perror("send");
                return -1;
            }
        } else if (bytes_sent == 0) {
            perror("send return 0");
            return -1;
        }

        total_sent += bytes_sent;
    }

    return (ssize_t)total_sent;
}
