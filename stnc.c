#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
char** create_sub_args(char* argv[], int i, int j){
    char** new_args = malloc(sizeof(void*) * (j-i) + 1) ;
    for(int k = i ; k < j ; k++){
        new_args[k-i] = argv[k] ;
    }
    new_args[j-i] = NULL ;
    return new_args ;
}

void print_man(){
    printf("Error in the command's format, please use one of the next formats: \n"
           "1. ./stnc -c <IP> <PORT> - for connecting to the desired IP and Port.\n"
           "2. ./stnc -s <PORT> - for creating the server on the localhost, at the desired Port.\n") ;
}

int main(int argc, char *argv[]) {
    int port ;
    if (strcmp(argv[1], "-c") == 0) {
        if (argc !=4) {
            print_man();
        } else {
            if( atoi(argv[3]) <= 0){
                print_man() ;
                return -1 ;
            }
            char** new_args = create_sub_args(argv, 2, 4) ;
            execvp("./client", new_args) ;
        }
    }else if(strcmp(argv[1], "-s") == 0){
        if(argc != 3){
            print_man() ;
        }else{
            if(atoi(argv[2]) <= 0) {
                print_man();
            }
            char** new_args = create_sub_args(argv, 2, 3) ;
            execvp("./server", new_args) ;
        }
    }else{
        print_man() ;
    }
}