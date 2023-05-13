#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#define CLIENT 0
#define SERVER 1
char** create_sub_args(char* argv[], int argc, int cl_or_serv){
    char **new_args ;
    if(cl_or_serv == CLIENT) {
        if(argc == 4) {
            new_args = malloc(sizeof(void *) * 4);
        }else{
            new_args = malloc(sizeof(void *) * 6);
        }
        new_args[0] = "./client";
        new_args[1] = argv[2];
        new_args[2] = argv[3];
        new_args[3] = NULL;
        if(argc == 7){
            new_args[3] = argv[5] ;
            new_args[4] = argv[6] ;
            new_args[5] = NULL ;
        }
    }else{
        new_args = malloc(sizeof(void *) * (argc));
        new_args[0] = "./server" ;
        new_args[1] = argv[2] ;
        new_args[2] = NULL ;
        if(argc >= 4){
            new_args[2] = argv[3] ;
            new_args[3] = NULL ;
        }
        if(argc == 5){
            new_args[3] = argv[4] ;
            new_args[4] = NULL ;
        }
    }
    return new_args ;
}

void print_man(){
    printf("Error in the command's format, please use one of the next formats: \n"
           "1. ./stnc -c <IP> <PORT> - for connecting to the desired IP and Port.\n"
           "2. ./stnc -s <PORT> - for creating the server on the localhost, at the desired Port.\n"
           "3. ./stnc -c <PORT> -p <type> <param> - for performance tests.\n"
           "<type> <param> combinations: \n"
           "\t* tcp ipv4\n"
           "\t* tcp ipv6\n"
           "\t* udp ipv4\n"
           "\t* udp ipv6\n"
           "\t* uds dgram\n"
           "\t* uds stream\n"
           "\t* mmap filename\n"
           "\t* pipe filename\n") ;
}

int main(int argc, char *argv[]) {
    char** new_args ;
    if (strcmp(argv[1], "-c") == 0) {
        if (argc !=4 && argc != 7) {
            print_man();
        } else {
            if( atoi(argv[3]) <= 0){
                print_man() ;
                return -1 ;
            }else{
                if(argc == 4){
                    new_args = create_sub_args(argv, argc, CLIENT) ;
                }else{
                    new_args = create_sub_args(argv, argc, CLIENT) ;
                }
            }
            execvp(new_args[0], new_args) ;
        }
    }else if(strcmp(argv[1], "-s") == 0){
        if(argc < 3 || argc > 5){
            print_man() ;
        }else{
            if(atoi(argv[2]) <= 0) {
                print_man();
            }
            new_args = create_sub_args(argv, argc, SERVER) ;
            execvp(new_args[0], new_args) ;
        }
    }else{
        print_man() ;
    }
}