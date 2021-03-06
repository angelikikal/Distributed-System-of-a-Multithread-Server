#ifndef SOCKETOPS_HPP
#define SOCKETOPS_HPP

#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

int LISTEN_PORT;

int create_socket(int port){
    int sock;
    if( (sock=socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("socket:");
        exit(3);
    }
    // reuse address
    int temp=1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &temp, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    struct sockaddr_in server;
    struct sockaddr* serverptr = (struct sockaddr*)&server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY); //bind to all interfaces
    server.sin_port = htons(port);
    if( bind(sock, serverptr, sizeof(server)) < 0){
        perror("bind: ");
        exit(4);
    }

    if( listen(sock,256) < 0){
        perror("listen:");
        exit(5);
    }

    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(sock, (struct sockaddr *)&sin, &len) == -1) {
        perror("getsockname failed");
    }
    else {
        LISTEN_PORT = ntohs(sin.sin_port);
    }
    return sock;
}


int connect_server(const char* server_ip, int server_port){
    int sock;
    if( (sock=socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("socket() connect:");
        exit(4);
    }
    //get address
    struct sockaddr_in server;
    struct sockaddr* serverptr=(struct sockaddr*)&server;
    struct hostent* rem;
    struct in_addr host_address;

    //translate host_or_ip
    if(isdigit(server_ip[0])){  //if ip
        inet_aton(server_ip,&host_address);
        rem = gethostbyaddr(&host_address,sizeof(host_address),AF_INET);
    }
    else{   //if host
        if((rem=gethostbyname(server_ip)) == NULL){
            perror("gethostbyname: ");
            exit(5);
        }
    }

    server.sin_family = AF_INET;
    memcpy(&server.sin_addr,rem->h_addr,rem->h_length);
    server.sin_port = htons(server_port);
    //connect to host
    if(connect(sock,serverptr,sizeof(server)) < 0){
        perror("server connect(): ");
        exit(6);
    }
    return sock;
}


char* read_sock(int sock){
    int buffer = 100;
    char* message = (char*)malloc(buffer*sizeof(char));
    int offset=0;

    //read message chunk by chunk
    while(read(sock, message+offset, buffer) == buffer){
      offset += buffer;
      message = (char*)realloc(message,offset+buffer);
    }
    return message;
}


void listen_for_commands(int command_socket){
    printf("worker%d listening for requests from server\n", getpid());
    //got connection, listen for commands
    while(1){
        int sock;
        if( (sock=accept(command_socket,NULL,NULL)) < 0){
            perror("accept on command socket ");
            return;
        }
        // got request. read it
        char* message = read_sock(sock);
        printf("\nworker got request: %s\n", message);  
        if(strcmp(message,"exit")==0) {
            free(message);
            return;
        }
        else {
            char* answer = (char*)malloc(BUFFER_SIZE);
            strcpy(answer, "worker's answer to ");
            strcat(answer, message);
            write(sock, answer, strlen(answer)+1);
            free(answer);
        }
        free(message);
        close(sock);
    };

    return;
}

#endif