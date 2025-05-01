#include <fcntl.h>
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
#include <signal.h>

#include "telemetry.h"
#include "conf.h"
#include "server_utils.h"

#define INIT_FDS_CAPACITY 10
#define BUFFER_SIZE 64

pthread_mutex_t sources_mutex = PTHREAD_MUTEX_INITIALIZER;
volatile bool server_running = true;


void* source_thread_function(void *arg);
void signal_handler(int signum);
void initialize_source(virtual_source *sources, size_t num_sources);

int main() {

    struct timespec last_send_time = {0};
    struct timespec current_time = {0};
    last_send_time.tv_sec = 1;
    current_time.tv_sec = 0;

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sa.sa_flags = 0;

    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    virtual_source *sources = NULL;
    size_t source_count = 0;
    
    pthread_t *source_threads = NULL;
 
    int listen_fd = -1, connect_fd = -1;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

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
    for (size_t i = 0; i < fds_capacity; ++i) {
        fds[i].fd = -1;
        fds[i].events = 0;
        fds[i].revents = 0;
    }
    printf("Allocated memory for fds\n");
    if (fds == NULL) {
        perror("malloc");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    fds[0].fd = listen_fd;
    fds[0].events = POLLIN;
    nfds = 1;

    source_threads = malloc(source_count * sizeof(pthread_t));
    if (source_threads == NULL) {
        perror("malloc");
        free(fds);
        free_sources_config(config);
        if (listen_fd >= 0) {
            close(listen_fd);
        }
        pthread_mutex_destroy(&sources_mutex);
        exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < source_count; ++i) {
        int ret = pthread_create(&source_threads[i], NULL, source_thread_function, (void *)&sources[i]);
        if (ret != 0) {
            server_running = false;
            for (size_t j = 0; j < i; ++j) {
                pthread_join(source_threads[j], NULL);
            }
            fprintf(stderr, "Failed to create thread for source %d: %s\n", sources[i].id, strerror(ret));
            free(source_threads);
            free(fds);
            free_sources_config(config);
            if (listen_fd >= 0) {
                close(listen_fd);
            }
            pthread_mutex_destroy(&sources_mutex);
            exit(EXIT_FAILURE);
        }
    }

    while(server_running) {

        int poll_count = poll(fds, nfds, -1);

        if (poll_count < 0) {
            if (errno == EINTR && server_running) {
                continue;
            }
            perror("poll failed");
            server_running = false;
            break;
        }

        for (nfds_t i = 0; i < nfds; i++) {

            if (fds[i].revents == 0) {
                continue;
            }

            if (fds[i].fd == listen_fd) {
                
                if (fds[i].revents & POLLIN) {

                    client_addr_len = sizeof(client_addr);
                    connect_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_addr_len);

                    if (connect_fd < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            perror("accept error");
                            break;
                        }
                    }

                    if (nfds >= fds_capacity) {
                        if (fds_realloc(&fds, &fds_capacity) < 0) {
                            fprintf(stderr, "Ошибка realloc fds, не можем добавить клиента fd=%d\n", connect_fd);
                            close(connect_fd);
                            continue;
                        }
                    }
                    

                    fds[nfds].fd = connect_fd;
                    fds[nfds].events = POLLOUT | POLLIN; 
                    nfds++;
                    printf("Клиент fd=%d добавлен. Всего дескрипторов: %lu\n", connect_fd, nfds);

                } else if (fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                    fprintf(stderr, "Критическая ошибка на слушающем сокете (fd=%d)! Завершение...\n", listen_fd);
                    server_running = false;
                }
            } else {
                int client_fd = fds[i].fd;

                if (fds[i].revents & POLLIN) {

                    char dummy_buffer;
                    printf("Client recv fds %d\n", fds[i].fd);
                    ssize_t bytes_received = recv(client_fd, &dummy_buffer, 1, 0);
                    if (bytes_received == 0) {
                        client_error(&nfds, &i, &fds);
                        printf("Клиент fd=%d отключился. Всего дескрипторов: %lu\n", client_fd, nfds);
                    } else if (bytes_received < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                        perror("recv error on client socket");
                        client_error(&nfds, &i, &fds);
                    } else {
                        printf("Received data %zu bytes from client fd %d\n", bytes_received, client_fd);
                        fds[i].revents = POLLOUT;
                    }

                } else if (fds[i].revents & POLLOUT) {
                    continue;
                } else {
                    client_error(&nfds, &i, &fds);
                    printf("Клиент fd=%d отключился. Всего дескрипторов: %lu\n", client_fd, nfds);
                }
            }
        }
        if (last_send_time.tv_sec - current_time.tv_sec >= 1) {
            clock_gettime(CLOCK_MONOTONIC, &current_time);

            unsigned char send_buffer[BUFFER_SIZE]; 

            for (nfds_t j = 1; j < nfds; ++j) {

                int client_fd = fds[j].fd;  
                if (client_fd < 0) continue;

                printf("FFF DATA fd %d (индекс j=%lu)...\n", client_fd, j);

                for (size_t k = 0; k < source_count; ++k) {
                    telemetry_data data;
                    bool is_active = false;

                    int ret = pthread_mutex_lock(&sources_mutex);
                    if (ret != 0) { fprintf(stderr, "ОШИБКА: lock mutex: %s\n", strerror(ret)); continue; }

                    if (sources[k].is_active) {
                        data = sources[k].data;
                        is_active = true;
                    }

                    ret = pthread_mutex_unlock(&sources_mutex);
                    if (ret != 0) { fprintf(stderr, "ОШИБКА: unlock mutex: %s\n", strerror(ret)); continue; }

                    if (!is_active) continue;

                    ssize_t bytes_to_send = serialize_telemetry_data(&data, send_buffer, sizeof(send_buffer));
                    if (bytes_to_send <= 0) {
                        continue;
                    }

                    ssize_t bytes_sent = send_all(client_fd, send_buffer, (size_t)bytes_to_send);
                    if (bytes_sent < 0 || (size_t)bytes_sent < (size_t)bytes_to_send) {
                        client_error(&nfds, &j, &fds);
                        break;
                    }

                }
            }
        }
        clock_gettime(CLOCK_MONOTONIC, &last_send_time);
    }

    printf("Exiting...\n");

    if (source_threads != NULL) {
        for (size_t i = 0; i < source_count; ++i) {
            int ret = pthread_join(source_threads[i], NULL);
            if (ret != 0) {
                fprintf(stderr, "Failed to join thread for source %d: %s\n", sources[i].id, strerror(ret));
            }
        }
        free(source_threads);
        source_threads = NULL;
    }

    for (nfds_t i = 1; i < nfds; i++) {
        close(fds[i].fd);
    }
    free(fds);
    
    if (listen_fd >= 0) {
        close(listen_fd);
    }

    free_sources_config(config);
    pthread_mutex_destroy(&sources_mutex);

    return 0;
}

void* source_thread_function(void *arg) {
    if (arg == NULL) {
        fprintf(stderr, "NULL argument passed to source thread function\n");
        pthread_exit((void *)-1);
    }

    virtual_source *source = (virtual_source *)arg;

    struct timespec sleep_req;
    struct timespec sleep_rem;
    sleep_req.tv_sec = source->update_interval_ms / 1000;
    sleep_req.tv_nsec = (source->update_interval_ms % 1000) * 1000000;

    while (server_running) {
        int ret = nanosleep(&sleep_req, &sleep_rem);
        while (ret == -1 && errno == EINTR) {
            sleep_req = sleep_rem;
            ret = nanosleep(&sleep_req, &sleep_rem);
        }
        if (ret == -1 && errno != EINTR) {
            perror("nanosleep");
            break;
        }
        if (!server_running) {
            break;
        }
        
        ret = pthread_mutex_lock(&sources_mutex);

        if (ret != 0) {
            fprintf(stderr, "Failed to lock mutex: %s\n", strerror(ret));
            break;
        }

        if (source->is_active) {
            update_source_reading(source);
        } 

        ret = pthread_mutex_unlock(&sources_mutex);

        if (ret != 0) {
            fprintf(stderr, "Failed to unlock mutex: %s\n", strerror(ret));
            break;
        }
    }

    pthread_exit(NULL);
}

void signal_handler(int signum) {
    server_running = false;
}

void initialize_source(virtual_source *sources, size_t num_sources) {
    pthread_mutex_lock(&sources_mutex);
    for (size_t i = 0; i < num_sources; ++i) {
        update_source_reading(&sources[i]);
    }
    pthread_mutex_unlock(&sources_mutex);
    printf("Inititial sensor\n");
}