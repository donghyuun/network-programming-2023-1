#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 20

//result field
#define ERROR 0
#define SUCCESS 1

//cmd field
#define REQUEST 0
#define RESPONSE 1
#define QUIT 2

//************2021428080 donghyun kim**************//

typedef struct {
    int cmd;//0: request, 1: response, 2: quit
    char addr[BUF_SIZE];//dotted-deciaml address(20byte)
    struct in_addr iaddr;//converted address of inet_aton()
    int result;//0: Error, 1: Success
}PACKET;

void error_handling(char* message);

int main(int argc, char*argv[]){
    //int num for file descriptor
    int serv_sock;
    int clnt_sock;

    //struct by "sys/socket.h" header file for IP, Port num
    struct sockaddr_in serv_addr;
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size;
    
    //packet variable for receving
    PACKET recv_packet;//PACKET include "struct" word also.

    if(argc != 2){
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    //1.generate socket
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if(serv_sock == 1) error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    //2.bind address to socket
    if(bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1){
        error_handling("bind() error");
    }else{
        printf("--------------------------\n");
        printf("Addreess Conversion Server\n");
        printf("--------------------------\n");
    }

    //3.listening
    if(listen(serv_sock, 5) == -1) error_handling("listen() error");

    //4.accept
    clnt_addr_size = sizeof(clnt_addr);
    clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
    if(clnt_sock == -1) error_handling("accept() error");

    //5.read &  write
    while(1){
        int rx_len = 0;
        rx_len = read(clnt_sock, &recv_packet, sizeof(PACKET));
        if(rx_len == 0) break;

        if(recv_packet.cmd == REQUEST){
            printf("[Rx] Received Dotted-Decimal Address: %s\n", recv_packet.addr);
            
            //convertion fail
            if(inet_aton(recv_packet.addr, &recv_packet.iaddr) == 0){
                printf("[Tx] Address conversion fail:(%s)\n", recv_packet.addr);
                printf("\n");

                //Do i have to initialize all member of PACKET?
                PACKET conv_error;
                conv_error.cmd = 1;
                conv_error.result = 0;

                write(clnt_sock, &conv_error, sizeof(conv_error));
            }
            //convertion success
            else{
                printf("inet_aton(%s) -> %#x\n", recv_packet.addr, recv_packet.iaddr.s_addr);
                
                PACKET conv_success;
                conv_success.cmd = 1;
                conv_success.result = 1;
                conv_success.iaddr = recv_packet.iaddr;

                write(clnt_sock, &conv_success, sizeof(conv_success));
                printf("[Tx] cmd: %d, iaddr: %#x, result: %d\n", conv_success.cmd, conv_success.iaddr.s_addr, conv_success.result);
                printf("\n");
            }
        }
        else if(recv_packet.cmd == QUIT){
            printf("[Rx] QUIT message received\n");
            printf("Server socket close and exit\n");

            close(serv_sock);
            close(clnt_sock);
            exit(1);
        }
        else {//handling exception
            printf("[Rx] Invalid command: %d\n", recv_packet.cmd);
            printf("\n");
            break;
        }
    }
    
}

//error handling
void error_handling(char* message){
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}