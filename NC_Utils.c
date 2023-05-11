#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define IP_ADDR "127.0.0.1"
#define IPV6_ADDR "::1"

#define TCP_IPV4 1
#define TCP_IPV6 2
#define UDP_IPV4 3
#define UDP_IPV6 4
#define UDS_DGRAM 5
#define UDS_STREAM 6
#define MMAP_FNAME 7
#define PIPE_FNAME 8

int yes = 1 ;


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

int create_tcp_ipv4_sock(char* ip, int port){
    int fd = -1 ;
    struct sockaddr_in serverAddress ;
    memset(serverAddress.sin_zero, 0, sizeof(serverAddress.sin_zero));
    serverAddress.sin_port = htons(port) ;
    serverAddress.sin_family = AF_INET ;
    inet_pton(AF_INET, ip, &(serverAddress.sin_addr));
    fd = socket(AF_INET, SOCK_STREAM, 0) ;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("Failed to set listening socket options.\n") ;
    }

    if(connect(fd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
        perror("Failed to connect to the server ") ;
        exit(-1) ;
    }
    printf("Connection established.") ;
    return fd ;
}

int create_tcp_ipv6_sock(char* ip, int port) {
    int fd = socket(AF_INET6, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("Failed to create socket");
    }
    struct sockaddr_in6 serverAddress_v6 ;
    memset(&serverAddress_v6, 0, sizeof(serverAddress_v6));
    serverAddress_v6.sin6_family = AF_INET6;
    serverAddress_v6.sin6_port = htons(port);

    if (inet_pton(AF_INET6, ip, &(serverAddress_v6.sin6_addr)) != 1) {
        perror("Invalid IPv6 address");
        close(fd);
    }

    if (connect(fd, (struct sockaddr *) &serverAddress_v6, sizeof(serverAddress_v6)) == -1) {
        perror("Failed to connect");
        close(fd);
    }
    return fd ;
}


int create_communication_fd(int strategy, char* data, int port){
    int fd =-1 ;
    switch (strategy) {
        case TCP_IPV4:
            fd = create_tcp_ipv4_sock(data, port) ;
            break;
        case TCP_IPV6:
            fd = create_tcp_ipv6_sock(data, port) ;
            break;
        case UDP_IPV4:
            break;
        case UDP_IPV6:
            break;
        case UDS_DGRAM:
            break;
        case UDS_STREAM:
            break;
        case MMAP_FNAME:
            break;
        case PIPE_FNAME:
            break;
        default:
            fd = 0 ;
    }
    return fd ;
}