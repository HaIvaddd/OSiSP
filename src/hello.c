#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <poll.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>

#include "telemetry.h"
#include "conf.h"

#define PORT 8080

#define BACKLOG_SIZE 10
#define BUFFER_SIZE 1024
#define INIT_FDS_CAPACITY 10

pthread_mutex_t sources_mutex = PTHREAD_MUTEX_INITIALIZER;
volatile bool server_running = true;



int socket_create();
void set_address(struct sockaddr_in *addr);
void bind_socket(int listen_fd, struct sockaddr_in *addr);
void listen_socket(int listen_fd);
void client_error(nfds_t *nfds, nfds_t *current_nfds , nfds_t *i, struct pollfd **fds);
void initialize_source(virtual_source *sources, size_t num_sources);

int fds_realloc(struct pollfd **fds_ptr, size_t *fds_capacity_ptr);

int main() {
    
    virtual_source *sources = NULL;
    size_t source_count = 0;
    
    pthread_t *source_threads = NULL;
 
    int listen_fd, connect_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char client_ip[INET_ADDRSTRLEN];
    char buffer[BUFFER_SIZE];

    srand(time(NULL));
    source_config config = load_sources_config();
    
    if (config.count == 0) {
        fprintf(stderr, "No sources configured\n");
        return EXIT_FAILURE;
    }

    source_count = config.count;
    sources = config.sources;
    initialize_source(sources, source_count);

    listen_fd = socket_create();
    set_address(&server_addr);
    bind_socket(listen_fd, &server_addr);
    listen_socket(listen_fd);

    printf("Listening on port %d with backlog size %d\n", PORT, BACKLOG_SIZE);

    struct pollfd *fds = NULL;
    nfds_t nfds = 0;
    size_t fds_capacity = INIT_FDS_CAPACITY;

    fds = malloc(fds_capacity * sizeof(struct pollfd));
    if (fds == NULL) {
        perror("malloc");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    fds[0].fd = listen_fd;
    fds[0].events = POLLIN;
    nfds = 1;

    while(server_running) {
        int poll_count = poll(fds, nfds, -1);
        if (poll_count < 0) {
            perror("poll");
            break;
        }

        nfds_t current_nfds = nfds;
        
        for (nfds_t i = 0; i < current_nfds; i++) {
            if (fds[i].revents == 0) {
                continue;
            }
            if (fds[i].fd == listen_fd) {
                if (fds[i].revents & POLLIN) {
                    printf("New connection request\n");
                    
                    client_addr_len = sizeof(client_addr);
                    connect_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_addr_len);
                    if (connect_fd < 0) {
                        perror("accept");
                        continue;
                    } else {
                        if (inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip)) == NULL) {
                            perror("inet_ntop");
                            close(connect_fd);
                            continue;
                        }
                        int client_port = ntohs(client_addr.sin_port);
                        printf("Accepted connection from %s:%d\n", client_ip, client_port);

                        if (nfds >= fds_capacity) {
                            if (fds_realloc(&fds, &fds_capacity) < 0) {
                                fprintf(stderr, "Failed to allocate memory for fds\n");
                                close(connect_fd);
                                continue;
                            }
                        }

                        fds[nfds].fd = connect_fd;
                        fds[nfds].events = POLLIN;
                        nfds++;
                    }

                } else if (fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                    fprintf(stderr, "Error on listening socket\n");
                    break;
                }
            } else {
                int client_fd = fds[i].fd;
                if (fds[i].revents & (POLLHUP | POLLERR | POLLNVAL)) {
                    if (fds[i].revents & POLLHUP) {
                        printf("Client disconnected\n");
                    } else {
                        fprintf(stderr, "Error on client socket\n");
                    }
                } else if (fds[i].revents & POLLIN) {
                    printf("Data available on client socket\n");
                    memset(buffer, 0, BUFFER_SIZE);
                    ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
                    if (bytes_received > 0) {
                        printf("Received %zd bytes: %s\n", bytes_received, buffer);
                        continue;
                    } else if (bytes_received == 0) {
                        printf("Client disconnected\n");
                    } else {
                        perror("recv");
                    }
                }
                client_error(&nfds, &current_nfds, &i, &fds);
            }
        }
    }
    printf("Exiting...\n");

    for (nfds_t i = 0; i < nfds; i++) {
        close(fds[i].fd);
    }
    free(fds);
    close(listen_fd);

    pthread_mutex_destroy(&sources_mutex);

    return 0;
}

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

void client_error(nfds_t *nfds, nfds_t *current_nfds , nfds_t *i, struct pollfd **fds) {

    close((*fds)[*i].fd);
    
    (*fds)[*i] = (*fds)[*nfds - 1];
    (*nfds) --;

    if (*i < *nfds) {
        (*i)--;
        (*current_nfds)--;
    }
}

void initialize_source(virtual_source *sources, size_t num_sources) {
    pthread_mutex_lock(&sources_mutex);
    for (size_t i = 0; i < num_sources; ++i) {
         update_source_reading(&sources[i]);
    }
    pthread_mutex_unlock(&sources_mutex);
    printf("Начальные показания инициализированы.\n");
}