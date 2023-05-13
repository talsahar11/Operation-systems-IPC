#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <poll.h>
#include <signal.h>
#include <sys/stat.h>
#include "NC_Utils.c"
#define MAX_PFD 6
#define GENERATED_DATA_LEN 100000000
int port ;
int stdin_fd = - 1, listening_fd4 = -1, listening_fd6 = -1, listening_fd_udss = -1, chat_fd = -1, communication_fd = - 1;
int pipefds[2] ;
int poll_size = 0 ;
struct pollfd *pfds ;
char *out_msg, *recv_buff, *test_buff ;
int is_to_send = 0 ;
int combination = 0 ;
int bytes_received = 0 ;
void* mapped_address = NULL ;
clock_t start, end ;
double total_time = -1 ;
double transfer_rate = 0 ;

void calculate_times_and_print(){
    total_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    transfer_rate = 100.0 / total_time ;
    printf("Time measured: %.2f Seconds ----- Transfer rate: %.2f Mbps", total_time, transfer_rate) ;
}

void handle_sigint(int signum) {
    exit_nicely_s(&pfds, poll_size) ;
    exit(0) ;
}

void reset(){
    bytes_received = 0 ;
    combination = 0 ;
    is_to_send = 0 ;
    mapped_address = NULL ;
    out_msg = malloc(1024) ;
    recv_buff = malloc(1024) ;
    test_buff = calloc(GENERATED_DATA_LEN, 1) ;
    close(communication_fd) ;
    end = 0 ;
}

void set_stdin_events(){
    stdin_fd = fileno(stdin);
    add_to_poll(&pfds, stdin_fd, POLLIN, 0, MAX_PFD, &poll_size);
}

void set_listening_sockets(int port){
    listening_fd4 = create_listening_socket_ipv4(port) ;
    listening_fd6 = create_listening_socket_ipv6(port) ;
    listening_fd_udss = create_listening_socket_udss() ;
    add_to_poll(&pfds, listening_fd4, POLLIN, 0, MAX_PFD, &poll_size) ;
    add_to_poll(&pfds, listening_fd6, POLLIN, 0, MAX_PFD, &poll_size) ;
    add_to_poll(&pfds, listening_fd_udss, POLLIN, 0, MAX_PFD, &poll_size) ;
}

int set_combination(){
    if(strcmp(recv_buff, "tcp ipv4") == 0) {
        combination = TCP_IPV4;
    }else if(strcmp(recv_buff, "tcp ipv6") == 0){
        combination = TCP_IPV6 ;
    }else if(strcmp(recv_buff, "udp ipv4") == 0){
        combination = UDP_IPV4 ;
    }else if(strcmp(recv_buff, "udp ipv6") == 0){
        combination = UDP_IPV6 ;
    }else if(strcmp(recv_buff, "uds dgram") == 0){
        combination = UDS_DGRAM ;
    }else if(strcmp(recv_buff, "uds stream") == 0){
        combination = UDS_STREAM ;
    }else if(strcmp(recv_buff, "mmap ready") == 0){
        combination = MMAP_FNAME ;
    }else if(strcmp(recv_buff, "pipe filename") == 0){
        combination = PIPE_FNAME ;
    }
    return combination ;
}

void check_for_requests(){

    if(combination == 0){
        if(set_combination()){
            if(combination == TCP_IPV4){
                strcpy(out_msg, IP_ADDR) ;
                is_to_send = 1 ;
            }else if(combination == TCP_IPV6){
                strcpy(out_msg, IPV6_ADDR) ;
                is_to_send = 1 ;
            }else if(combination == UDP_IPV4){
                communication_fd = create_udp_ipv4_socket(NULL) ;
                char* port = get_sock_port(communication_fd) ;
                strcpy(out_msg, port) ;
                is_to_send = 1 ;
                add_to_poll(&pfds, communication_fd, POLLIN, 0, MAX_PFD, &poll_size) ;
            }else if(combination == UDP_IPV6){
                communication_fd = create_udp_ipv6_socket(NULL) ;
                char* port = get_sock_port(communication_fd) ;
                strcpy(out_msg, port) ;
                is_to_send = 1 ;
                add_to_poll(&pfds, communication_fd, POLLIN, 0, MAX_PFD, &poll_size) ;
            }else if(combination == UDS_STREAM){
                strcpy(out_msg, UDS_LISTENING_PATH) ;
                is_to_send = 1 ;
            }else if(combination == UDS_DGRAM){
                communication_fd = create_udsd_socket(NULL) ;
                strcpy(out_msg, UDS_COMM_PATH) ;
                is_to_send = 1 ;
                add_to_poll(&pfds, communication_fd, POLLIN, 0, MAX_PFD, &poll_size) ;
            }else if(combination == MMAP_FNAME){
                start = clock() ;
                mapped_address = mmap_file_s() ;
                memcpy(test_buff, mapped_address, GENERATED_DATA_LEN) ;
                munmap(mapped_address, GENERATED_DATA_LEN) ;
                end = clock() ;
                calculate_times_and_print() ;
            }else if(combination == PIPE_FNAME){
                if(mkfifo(FIFO_PATH, 0666) == -1){
                    perror("FIFO: \n") ;
                }

                strcpy(out_msg, FIFO_PATH) ;
                communication_fd = open(FIFO_PATH, O_RDONLY | O_NDELAY) ;
                add_to_poll(&pfds, communication_fd, POLLIN, 0, MAX_PFD, &poll_size) ;
                is_to_send = 1 ;
            }
        }

    }
}


int main(int argc, char* argv[]){
    signal(SIGINT, handle_sigint);
    port = atoi(argv[1]) ;
    pfds = malloc(sizeof *pfds * MAX_PFD);
    reset() ;
    set_stdin_events() ;
    set_listening_sockets(port) ;
    while(1){
        int poll_count = poll(pfds, poll_size, -1) ;
        if(poll_count < 0){
            perror("Poll error: ");
            exit(-1);
        } else {
            for (int i = 0; i < poll_size; i++) {
                int current_fd = pfds[i].fd ;
                if(pfds[i].revents & POLLIN){
                    if(current_fd == listening_fd4 || current_fd == listening_fd6){
                        int newfd = accept_socket(current_fd) ;
                        printf("New connection established.. \n") ;
                        if(poll_size == 4){
                            chat_fd = newfd ;
                            add_to_poll(&pfds, chat_fd, POLLIN, POLLOUT, MAX_PFD, &poll_size) ;
                        }else {
                            communication_fd = newfd ;
                            add_to_poll(&pfds, communication_fd, POLLIN, 0, MAX_PFD, &poll_size) ;
                            printf("%d added to poll, events: %d\n", newfd, pfds[poll_size-1].events);
                        }
                    }
                    if(current_fd == listening_fd_udss){
                        communication_fd = accept_udss_socket(listening_fd_udss) ;
                            add_to_poll(&pfds, communication_fd, POLLIN, 0, MAX_PFD, &poll_size) ;
                            printf("%d added to poll, events: %d\n", communication_fd, pfds[poll_size-1].events);
                    }
                    if(current_fd == chat_fd){
                        int nbytes = recv(pfds[i].fd, recv_buff, 1023, 0);
                        int sender_fd = pfds[i].fd;
                        if (nbytes <= 0) {
                            if (nbytes == 0) {
                                printf("pollserver: socket %d hung up\n", sender_fd);
                            } else {
                                perror("Failed receiving from client");
                            }
                            close(pfds[i].fd); // Bye!
                            combination = 0 ;
                            remove_from_poll(&pfds, &poll_size, chat_fd);
                            remove_from_poll(&pfds, &poll_size, communication_fd);
                        } else {
                            recv_buff[nbytes] = '\0';
                            printf("%s\n", recv_buff) ;
                            if(is_clock_msg(recv_buff)){
                                start = clock() ;
                            }else {
                                check_for_requests();
                            }
                        }
                    }else if(current_fd == communication_fd) {
                        int nbytes = -1 ;
                        if(combination == PIPE_FNAME){
                            nbytes = read(communication_fd, test_buff, GENERATED_DATA_LEN) ;
                        }else {
                            nbytes = recv(pfds[i].fd, test_buff, GENERATED_DATA_LEN, 0);
                        }
                        bytes_received += nbytes;
                        int sender_fd = pfds[i].fd;
                        if (nbytes <= 0) {
                            if (nbytes == 0) {
                                printf("pollserver: shitty socket %d hung up\n", sender_fd);
                            } else {
                                perror("Failed receiving from client");
                            }
                            close(pfds[i].fd); // Bye!
                            remove_from_poll(&pfds, &poll_size, communication_fd);
                        } else {
                            test_buff[nbytes] = '\0';
                            if(combination == UDP_IPV4 || combination == UDP_IPV6 || combination == UDS_DGRAM){
                            strcpy(out_msg, "ACK") ;
                            is_to_send = 1 ;
                            }
                        }
                        if(bytes_received == GENERATED_DATA_LEN){
                            end = clock() ;
                            calculate_times_and_print() ;
                            reset() ;
                        }
                    }else if(current_fd == stdin_fd){
                        int ret = read(pfds[i].fd, out_msg, 1023);
                        if (ret > 1) {
                            out_msg[ret-1] = '\0';
                            is_to_send = 1 ;
                        } else if (ret < 0) {
                            perror("Error reading from the input: ") ;
                        }
                    }
                }else if(pfds[i].revents & POLLOUT){
                    if(is_to_send && current_fd == chat_fd){
                        int bytes = send(pfds[i].fd, out_msg, strlen(out_msg), 0) ;
                        is_to_send = 0 ;
                        memset(out_msg, '\0', 1024) ;
                    }
                }
            }
        }
    }
}