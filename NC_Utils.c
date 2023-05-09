#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#define IP_ADDR "127.0.0.1"
#define IPV6_ADDR "::1"


void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void add_to_poll(struct pollfd *pfds[], int fd, int poll_in, int poll_out, int max_pfd, int *poll_size){
    if(*poll_size == max_pfd){
        printf("Failed adding fd to the poll, poll is full.\n") ;
    }else{
        (*pfds)[*poll_size].fd = fd ;
        if(poll_in == POLLIN){
            if(poll_out == POLLOUT){
                (*pfds)[*poll_size].events = POLLIN | POLLOUT ;
            }else {
                (*pfds)[*poll_size].events = POLLIN;
            }
        }else if(poll_out == POLLOUT){
            (*pfds)[*poll_size].events = POLLOUT ;
        }
        (*poll_size)++ ;
    }
}

void remove_from_poll(struct pollfd *pfds[], int *poll_size, int index){
    (*pfds)[index] = (*pfds)[*poll_size - 1] ;
    *poll_size-- ;
}


int create_listening_socket_ipv4(int port){
    int yes=1;        // For setsockopt() SO_REUSEADDR, below
    struct sockaddr_in serverAddress ;
    memset(serverAddress.sin_zero, 0, sizeof(serverAddress.sin_zero));
    serverAddress.sin_port = htons(port) ;
    serverAddress.sin_family = AF_INET ;
    inet_pton(AF_INET, IP_ADDR, &(serverAddress.sin_addr));
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0) ;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("Failed to set listening socket options.\n") ;
        return -1 ;
    }

    if(bind(sock_fd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("Failed to bind the listening socket.");
        return -1;
    }
    if(listen(sock_fd, 1) == -1){
        perror("Failed to listen ") ;
    }
    printf("Listening for connections... \n") ;
    return sock_fd ;
}
int create_listening_socket_ipv6(int port) {
    int yes = 1; // For setsockopt() SO_REUSEADDR, below
    struct sockaddr_in6 serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin6_port = htons(port);
    serverAddress.sin6_family = AF_INET6;
    inet_pton(AF_INET6, IPV6_ADDR, &(serverAddress.sin6_addr));
    int sock_fd = socket(AF_INET6, SOCK_STREAM, 0);
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("Failed to set listening socket options.\n");
        return -1;
    }

    if (bind(sock_fd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("Failed to bind the listening socket.");
        return -1;
    }
    if (listen(sock_fd, 1) == -1) {
        perror("Failed to listen");
    }
    return sock_fd;
}

int accept_socket(int listening_fd){
    int newfd ;
    struct sockaddr_storage remoteaddr; // Client address
    char remoteIP[INET6_ADDRSTRLEN];

    socklen_t addrlen;
    addrlen = sizeof remoteaddr;
    newfd = accept(listening_fd,
                   (struct sockaddr *) &remoteaddr,
                   &addrlen);

    if (newfd == -1) {
        perror("Failed accepting new connection.");
    } else {
        printf("pollserver: new connection from %s on "
               "socket %d ----- IPV6\n",
               inet_ntop(remoteaddr.ss_family,
                         get_in_addr((struct sockaddr *) &remoteaddr),
                         remoteIP, INET6_ADDRSTRLEN),
               newfd);
    }
    return newfd ;
}