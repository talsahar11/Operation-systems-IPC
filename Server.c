#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include "NC_Utils.c"
#define MAX_PFD 5

int port ;
int stdin_fd = - 1, listening_fd4 = -1, listening_fd6 = -1, chat_fd = -1;
int poll_size = 0 ;
struct pollfd *pfds ;
int yes = 1 ;
char *out_msg, *recv_buff ;
int is_to_send ;
void set_stdin_events(){
    stdin_fd = fileno(stdin);
    add_to_poll(&pfds, stdin_fd, POLLIN, 0, MAX_PFD, &poll_size);
}

void set_listening_sockets(int port){
    listening_fd4 = create_listening_socket_ipv4(port) ;
    listening_fd6 = create_listening_socket_ipv6(port) ;
    add_to_poll(&pfds, listening_fd4, POLLIN, 0, MAX_PFD, &poll_size) ;
    add_to_poll(&pfds, listening_fd6, POLLIN, 0, MAX_PFD, &poll_size) ;
}

int main(int argc, char* argv[]){
    port = atoi(argv[0]) ;
    out_msg = malloc(1024) ;
    recv_buff = malloc(1024) ;
    pfds = malloc(sizeof *pfds * MAX_PFD);
    set_stdin_events() ;
    set_listening_sockets(port) ;
    printf("POLLSIZE: %d\n", poll_size) ;
    while(1){
        int poll_count = poll(pfds, poll_size, -1) ;
        if(poll_count < 0){

            perror("Poll error: ");
            exit(-1);
        } else {
            for (int i = 0; i < poll_size; i++) {
                int current_fd = pfds[i].fd ;
                if(pfds[i].revents & POLLIN){
                    if(current_fd == listening_fd4){
                        printf("SHit polled + %d\n", i) ;
                        int newfd = accept_socket(current_fd) ;
                        printf("New connection established.. \n") ;
                        if(poll_size == 3){
                            chat_fd = newfd ;
                            add_to_poll(&pfds, chat_fd, POLLIN, POLLOUT, MAX_PFD, &poll_size) ;
                        }else {
                            add_to_poll(&pfds, newfd, POLLIN, 0, MAX_PFD, &poll_size) ;
                        }
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
                            remove_from_poll(&pfds, &poll_size, i);
                        } else {
                            recv_buff[nbytes] = '\0';
                            printf("%s\n", recv_buff);
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
                        printf("Bytes sent: %d\n", bytes) ;
                        is_to_send = 0 ;
                        memset(out_msg, '\0', 1024) ;
                    }
                }
            }
        }
    }
}