#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include "NC_Utils.c"
#define MAX_PFD 3
#define CHUNKSIZE 8192
char *ip;
int port;
int stdin_fd = -1, chat_fd = -1, communication_fd = -1;
int poll_size = 0;
struct pollfd *pfds;
char *out_msg, *recv_buff, *test_buff;
int is_to_send;
char *type, *param;
int combination = 0;
int is_test = 0;
int is_end = 0;
int is_acked = 1;
int udp_port = 0;
int offset = 0;
void* mapped_address = NULL ;
void set_stdin_events()
{
    stdin_fd = fileno(stdin);
    add_to_poll(&pfds, stdin_fd, POLLIN, 0, MAX_PFD, &poll_size);
}

void generate_data()
{
    int i = 0;
    test_buff = (char *)calloc(GENERATED_DATA_LEN, 1);
    while (i < GENERATED_DATA_LEN)
    {
        test_buff[i] = (rand() % 100) + 1;
        i++;
    }
}

void set_chat_socket(char *ip, int port)
{
    struct sockaddr_in serverAddress;
    memset(serverAddress.sin_zero, 0, sizeof(serverAddress.sin_zero));
    serverAddress.sin_port = htons(port);
    serverAddress.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &(serverAddress.sin_addr));
    chat_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (setsockopt(chat_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
        perror("Failed to set listening s100000000ocket options.\n");
    }

    if (connect(chat_fd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        perror("Failed to connect to the server ");
        exit(-1);
    }
    printf("Connection established.\n");
    add_to_poll(&pfds, chat_fd, POLLIN, POLLOUT, MAX_PFD, &poll_size);
}
void set_combination()
{
    if (strcmp(type, "tcp") == 0)
    {
        if (strcmp(param, "ipv4") == 0)
        {
            combination = TCP_IPV4;
        }
        else
        {
            combination = TCP_IPV6;
        }
    }
    else if (strcmp(type, "udp") == 0)
    {
        if (strcmp(param, "ipv4") == 0)
        {
            combination = UDP_IPV4;
        }
        else
        {
            combination = UDP_IPV6;
        }
    }
    else if (strcmp(type, "uds") == 0)
    {
        if (strcmp(param, "dgram") == 0)
        {
            combination = UDS_DGRAM;
        }
        else
        {
            combination = UDS_STREAM;
        }
    }
    else if (strcmp(type, "mmap") == 0)
    {
        combination = MMAP_FNAME;
    }
    else
    {
        combination = PIPE_FNAME;
    }
}

void handle_response(char *data)
{
    printf("Combination %d\n", combination);
    if (!is_end)
    {
        switch (combination)
        {
        case TCP_IPV4:
            communication_fd = create_communication_fd(combination, data, port);
            break;
        case TCP_IPV6:
            communication_fd = create_communication_fd(combination, data, port);
            break;
        case UDP_IPV4:
            communication_fd = create_communication_fd(combination, data, 0);
            break;
        case UDP_IPV6:
            communication_fd = create_communication_fd(combination, data, 0);
            break;
        case UDS_DGRAM:
            communication_fd = create_communication_fd(combination, data, 0);
            break;
        case UDS_STREAM:
            communication_fd = create_communication_fd(combination, data, 0);
            break;
        case MMAP_FNAME:
            communication_fd = create_communication_fd(combination, data, 0);
            break;
        }
        add_to_poll(&pfds, communication_fd, 0, POLLOUT, MAX_PFD, &poll_size);
        is_end = 1;
    }
    else
    {
        if (strcmp(recv_buff, "ACK") == 0)
        {
            is_acked = 1;
        }
    }
}

int main(int argc, char *argv[])
{
    ip = argv[1];
    port = atoi(argv[2]);
    out_msg = malloc(1024);
    recv_buff = malloc(1024);
    test_buff = malloc(GENERATED_DATA_LEN);

    pfds = malloc(sizeof *pfds * MAX_PFD);
    set_stdin_events();
    set_chat_socket(ip, port);
    if (argc == 5)
    {
        generate_data();
        type = argv[3];
        param = argv[4];
        is_test = 1;
        set_combination();
        if(combination == MMAP_FNAME){
            mapped_address = mmap_file_c() ;
            memcpy(mapped_address, test_buff, GENERATED_DATA_LEN) ;
            strcpy(out_msg, "mmap ready") ;
        }else{
        char msg[20];
        strcpy(msg, type);
        strcat(msg, " ");
        strcat(msg, param);
        strcpy(out_msg, msg);
        }
        is_to_send = 1;
        
    }

    while (1)
    {
        int poll_count = poll(pfds, poll_size, -1);
        if (poll_count < 0)
        {
            perror("Poll error: ");
            exit(-1);
        }
        else
        {
            for (int i = 0; i < poll_size; i++)
            {
                int current_fd = pfds[i].fd;
                if (pfds[i].revents & POLLIN)
                {
                    if (current_fd == chat_fd)
                    {
                        int nbytes = recv(pfds[i].fd, recv_buff, 1023, 0);
                        int sender_fd = pfds[i].fd;
                        if (nbytes <= 0)
                        {
                            if (nbytes == 0)
                            {
                                printf("pollserver: socket %d hung up\n", sender_fd);
                            }
                            else
                            {
                                perror("Failed receiving from client");
                            }
                            close(pfds[i].fd); // Bye!
                            remove_from_poll(&pfds, &poll_size, i);
                        }
                        else
                        {
                            recv_buff[nbytes] = '\0';
                            if (is_test)
                            {
                                printf("Handling res\n");
                                handle_response(recv_buff);
                            }
                            printf("%s\n", recv_buff);
                        }
                    }
                    else if (current_fd == stdin_fd)
                    {
                        int ret = read(pfds[i].fd, out_msg, 1023);
                        if (ret > 1)
                        {
                            out_msg[ret - 1] = '\0';
                            is_to_send = 1;
                        }
                        else if (ret < 0)
                        {
                            perror("Error reading from the input: ");
                        }
                    }
                }
                else if (pfds[i].revents & POLLOUT)
                {
                    if (is_to_send && current_fd == chat_fd)
                    {
                        int bytes = send(pfds[i].fd, out_msg, strlen(out_msg), 0);
                        is_to_send = 0;
                        memset(out_msg, '\0', 1024);
                    }
                    if (is_test)
                    {
                        if (current_fd == communication_fd)
                        {

                            if (is_acked)
                            {
                                int bytes = send(pfds[i].fd, test_buff + offset, (offset + CHUNKSIZE <= GENERATED_DATA_LEN) ? CHUNKSIZE : GENERATED_DATA_LEN - offset, 0);
                                if (combination == UDP_IPV4 || combination == UDP_IPV6 || combination == UDS_DGRAM)
                                {
                                    is_acked = 0;
                                }
                                if (bytes == -1)
                                {
                                    perror("Failed sending: ");
                                    return 0;
                                }
                                else
                                {
                                    offset += bytes;
                                }
                                printf("sent %d bytes, Total of: %d!\n", bytes, offset);
                            }
                            if (offset == GENERATED_DATA_LEN)
                            {
                                printf("closingggg\n");

                                if (remove("/tmp/server_sockk") == 0 && remove("/tmp/uds_comm") == 0)
                                {
                                    printf("Socket file deleted successfully.\n");
                                }
                                else
                                {
                                    printf("Unable to delete the socket file.\n");
                                }
                                close(communication_fd);
                            }
                        }
                    }
                }
            }
        }
    }
}