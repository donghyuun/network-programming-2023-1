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

void error_handling(char *message);

int main(int argc, char *argv[]){
    int sock;
    struct sockaddr_in serv_addr;
    
    if(argc != 3){
        printf("Usage: %s <IP> <Port>\n", argv[0]);
        exit(1);
    }

    //1.generate socket
    sock = socket(PF_INET, SOCK_STREAM, 0);//IPv4 & TCP
    if(sock == -1) error_handling("socket() error");
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    //2.conncet
    if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1){
        error_handling("connect() error!");
    }

    //3. read & write
    while(1){        
        //----send packet----
        PACKET send_message;
        printf("Input dotted-decimal adress: ");
        scanf("%s", send_message.addr);

        //quit
        if(strcmp(send_message.addr, "quit") == 0){
            send_message.cmd = QUIT;
            write(sock, &send_message, sizeof(send_message));
            printf("[Tx] cmd: %d(QUIT)\n", send_message.cmd);
            printf("Client socket close and exit\n");
            exit(1);
        }
        //send
        else{
            send_message.cmd = REQUEST;
            write(sock, &send_message, sizeof(send_message));
            printf("[Tx] cmd: %d, addr: %s\n", send_message.cmd, send_message.addr);

            //receive packet
            PACKET recv_message;
            int len = read(sock, &recv_message, sizeof(recv_message));
            
            if(len == -1){
                error_handling("client read() error\n");
            }
            else if(recv_message.result == 1){//receive success
                printf("[Rx] cmd: %d, Adress converstion: %#x (result: %d)\n", recv_message.cmd, recv_message.iaddr.s_addr, recv_message.result);
                printf("\n");
            }
            else{//receive fail
                printf("[Rx] cmd: %d, Adress converstion fail! (result: %d)\n", recv_message.cmd, recv_message.result);
                printf("\n");
            }
        }

    }
}

//error handling
void error_handling(char* message){
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}