#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>

#define GENERATED_DATA_LEN 100000000
#define IP_ADDR "127.0.0.1"
#define IPV6_ADDR "::1"
#define UDS_LISTENING_PATH "/tmp/server_sockk"
#define UDS_COMM_PATH "/tmp/uds_comm"
#define MMAP_PATH "mmap_p"
#define TCP_IPV4 1
#define TCP_IPV6 2
#define UDP_IPV4 3
#define UDP_IPV6 4
#define UDS_DGRAM 5
#define UDS_STREAM 6
#define MMAP_FNAME 7
#define PIPE_FNAME 8
int yes = 1;

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

void add_to_poll(struct pollfd *pfds[], int fd, int poll_in, int poll_out, int max_pfd, int *poll_size)
{
    if (*poll_size == max_pfd)
    {
        printf("Failed adding fd to the poll, poll is full.\n");
    }
    else
    {
        (*pfds)[*poll_size].fd = fd;
        if (poll_in == POLLIN)
        {
            if (poll_out == POLLOUT)
            {
                (*pfds)[*poll_size].events = POLLIN | POLLOUT;
            }
            else
            {
                (*pfds)[*poll_size].events = POLLIN;
            }
        }
        else if (poll_out == POLLOUT)
        {
            (*pfds)[*poll_size].events = POLLOUT;
        }
        (*poll_size)++;
    }
}

void remove_from_poll(struct pollfd *pfds[], int *poll_size, int index)
{
    (*pfds)[index] = (*pfds)[*poll_size - 1];
    *poll_size--;
}

int create_listening_socket_ipv4(int port)
{
    int yes = 1; // For setsockopt() SO_REUSEADDR, below
    struct sockaddr_in serverAddress;
    memset(serverAddress.sin_zero, 0, sizeof(serverAddress.sin_zero));
    serverAddress.sin_port = htons(port);
    serverAddress.sin_family = AF_INET;
    inet_pton(AF_INET, IP_ADDR, &(serverAddress.sin_addr));
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
        perror("Failed to set listening socket options.\n");
        return -1;
    }

    if (bind(sock_fd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        perror("Failed to bind the listening socket.");
        return -1;
    }
    if (listen(sock_fd, 1) == -1)
    {
        perror("Failed to listen ");
    }
    printf("Listening for connections... shity\n");
    return sock_fd;
}
int create_listening_socket_ipv6(int port)
{
    int yes = 1; // For setsockopt() SO_REUSEADDR, below
    struct sockaddr_in6 serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin6_port = htons(port);
    serverAddress.sin6_family = AF_INET6;
    inet_pton(AF_INET6, IPV6_ADDR, &(serverAddress.sin6_addr));
    int sock_fd = socket(AF_INET6, SOCK_STREAM, 0);
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
        perror("Failed to set listening socket options.\n");
        return -1;
    }

    if (bind(sock_fd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        perror("Failed to bind the listening socket.");
        return -1;
    }
    if (listen(sock_fd, 1) == -1)
    {
        perror("Failed to listen");
    }
    printf("listening for ipv6....\n");
    return sock_fd;
}

int create_listening_socket_udss()
{
    int fd;
    struct sockaddr_un serverAddr;

    // Create a UDS socket
    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0)
    {
        perror("Error in socket");
        exit(EXIT_FAILURE);
    }

    // Configure server address settings
    serverAddr.sun_family = AF_UNIX;
    strcpy(serverAddr.sun_path, UDS_LISTENING_PATH);

    printf("listening for shit....\n");
    // Bind the socket to the server address
    if (bind(fd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("Error in bind");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(fd, 1) < 0)
    {
        perror("Error in listen");
        exit(EXIT_FAILURE);
    }
    return fd;
}

int accept_socket(int listening_fd)
{
    int newfd;
    struct sockaddr_storage remoteaddr; // Client address
    char remoteIP[INET6_ADDRSTRLEN];

    socklen_t addrlen;
    addrlen = sizeof remoteaddr;
    newfd = accept(listening_fd,
                   (struct sockaddr *)&remoteaddr,
                   &addrlen);

    if (newfd == -1)
    {
        perror("Failed accepting new connection.");
    }
    else
    {
        printf("pollserver: new connection from %s on "
               "socket %d\n",
               inet_ntop(remoteaddr.ss_family,
                         get_in_addr((struct sockaddr *)&remoteaddr),
                         remoteIP, INET6_ADDRSTRLEN),
               newfd);
    }
    return newfd;
}

int accept_udss_socket(int listening_fd)
{
    // Accept a client connection
    int client_fd = accept(listening_fd, NULL, NULL);
    if (client_fd < 0)
    {
        perror("Error in accept");
        exit(EXIT_FAILURE);
    }
    return client_fd;
}

int create_tcp_ipv4_sock(char *ip, int port)
{
    int fd = -1;
    struct sockaddr_in serverAddress;
    memset(serverAddress.sin_zero, 0, sizeof(serverAddress.sin_zero));
    serverAddress.sin_port = htons(port);
    serverAddress.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &(serverAddress.sin_addr));
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
        perror("Failed to set listening socket options.\n");
    }

    if (connect(fd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        perror("Failed to connect to the server ");
        exit(-1);
    }
    printf("Connection established.");
    return fd;
}

int create_tcp_ipv6_sock(char *ip, int port)
{
    int fd = socket(AF_INET6, SOCK_STREAM, 0);
    if (fd == -1)
    {
        perror("Failed to create socket");
    }
    struct sockaddr_in6 serverAddress_v6;
    memset(&serverAddress_v6, 0, sizeof(serverAddress_v6));
    serverAddress_v6.sin6_family = AF_INET6;
    serverAddress_v6.sin6_port = htons(port);

    if (inet_pton(AF_INET6, ip, &(serverAddress_v6.sin6_addr)) != 1)
    {
        perror("Invalid IPv6 address");
        close(fd);
    }

    if (connect(fd, (struct sockaddr *)&serverAddress_v6, sizeof(serverAddress_v6)) == -1)
    {
        perror("Failed to connect");
        close(fd);
    }
    return fd;
}

int create_udp_ipv4_socket(char *data)
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1)
    {
        perror("Failed to create socket");
    }
    struct sockaddr_in my_addr;
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(0); // Specify 0 to let the OS assign a free port
    my_addr.sin_addr.s_addr = inet_addr(IP_ADDR);

    if (bind(fd, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0)
    {
        perror("Error in bind");
        exit(EXIT_FAILURE);
    }
    if (data != NULL)
    {
        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(atoi(data));
        serverAddr.sin_addr.s_addr = inet_addr(IP_ADDR);

        // Connect to the server
        if (connect(fd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
        {
            perror("Error in connect");
            exit(EXIT_FAILURE);
        }
    }
    return fd;
}

int create_udp_ipv6_socket(char *data)
{
    int fd = socket(AF_INET6, SOCK_DGRAM, 0);
    if (fd == -1)
    {
        perror("Failed to create socket");
    }
    struct sockaddr_in6 my_adder;
    memset(&my_adder, 0, sizeof(my_adder));
    my_adder.sin6_family = AF_INET6;
    my_adder.sin6_port = htons(0); // Specify 0 to let the OS assign a free port
    if (inet_pton(AF_INET6, IPV6_ADDR, &(my_adder.sin6_addr)) != 1)
    {
        perror("Invalid IPv6 address");
        close(fd);
    }

    if (bind(fd, (struct sockaddr *)&my_adder, sizeof(my_adder)) < 0)
    {
        perror("Error in bind");
        exit(EXIT_FAILURE);
    }
    if (data != NULL)
    {
        struct sockaddr_in6 serverAddr;
        serverAddr.sin6_family = AF_INET6;
        serverAddr.sin6_port = htons(atoi(data));
        if (inet_pton(AF_INET6, IPV6_ADDR, &(serverAddr.sin6_addr)) != 1)
        {
            perror("Invalid IPv6 address");
            close(fd);
        }
        // Connect to the server
        if (connect(fd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
        {
            perror("Error in connect");
            exit(EXIT_FAILURE);
        }
    }

    return fd;
}

char *get_sock_port(int fd)
{
    struct sockaddr_in allocatedAddr;
    socklen_t addrLen = sizeof(allocatedAddr);
    getsockname(fd, (struct sockaddr *)&allocatedAddr, &addrLen);
    char *port_str = malloc(6);
    sprintf(port_str, "%d", ntohs(allocatedAddr.sin_port));
    printf("Port: %s\n", port_str);
    return port_str;
}

int create_udsd_socket(char *path)
{
    int fd;
    struct sockaddr_un serverAddr;

    // Create a UDS socket
    fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (fd < 0)
    {
        perror("Error in socket");
        exit(EXIT_FAILURE);
    }
    if (path == NULL)
    {
        // Configure server address settings
        serverAddr.sun_family = AF_UNIX;
        strcpy(serverAddr.sun_path, UDS_COMM_PATH);

        // Bind the socket to the server address
        if (bind(fd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
        {
            perror("Error in bind");
            exit(EXIT_FAILURE);
        }
    }else{
        serverAddr.sun_family = AF_UNIX;
        strcpy(serverAddr.sun_path, path);
        // Connect to the server
        if (connect(fd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
        {
            perror("Error in connect");
            exit(EXIT_FAILURE);
        }
    }
    return fd;
}

int create_udss_socket(char *path)
{
    int fd;
    struct sockaddr_un serverAddr;

    // Create a UDS socket
    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0)
    {
        perror("Error in socket");
        exit(EXIT_FAILURE);
    }

    // Configure server address settings
    serverAddr.sun_family = AF_UNIX;
    strcpy(serverAddr.sun_path, path);

    // Connect to the server
    if (connect(fd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("Error in connect");
        exit(EXIT_FAILURE);
    }
    return fd;
}

void* mmap_file_c(int fd){

    void* addr = mmap(NULL, GENERATED_DATA_LEN, PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr != MAP_FAILED) {
        printf("Memory mapping successful.\n");
//            munmap(addr, 1000);
    } else {
        perror("mmap");
        close(fd); // Close the file descriptor before exiting
        return NULL ;
    }

        close(fd); // Close the file descriptor after memory mapping

    printf("done with that shit!\n") ;
    return addr;
}

void* mmap_file_s(){
    int fd = open(MMAP_PATH, O_RDONLY | O_CREAT);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    void* mappedAddr = mmap(NULL, GENERATED_DATA_LEN, PROT_READ, MAP_SHARED, fd, 0);
    if (mappedAddr == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    return mappedAddr ;
}

int create_communication_fd(int strategy, char *data, int port)
{
    int fd = -1;
    switch (strategy)
    {
        case TCP_IPV4:
            fd = create_tcp_ipv4_sock(data, port);
            break;
        case TCP_IPV6:
            fd = create_tcp_ipv6_sock(data, port);
            break;
        case UDP_IPV4:
            fd = create_udp_ipv4_socket(data);
            break;
        case UDP_IPV6:
            fd = create_udp_ipv6_socket(data);
            break;
        case UDS_DGRAM:
            fd = create_udsd_socket(data);
            break;
        case UDS_STREAM:
            fd = create_udss_socket(data);
            break;
        case MMAP_FNAME:

            break;
        case PIPE_FNAME:
            break;
        default:
            fd = 0;
    }
    return fd;
}