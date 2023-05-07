#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
//optoin value
char *option[11] = {
    "", 
    "SO_SNDBUF", 
    "SO_RCVBUF", 
    "SO_REUSEADDR", 
    "SO_KEEPALIVE",
    "SO_BROADCAST",
    "IP_TOS",
    "IP_TTL",
    "TCP_NODELAY",
    "TCP_MAXSEG",
    "Quit"
};
 
typedef struct{
    int level;
    int option;
    int optval; //requested option value
    int result; //process success(0) or failed(-1) 
}SO_PACKET;


void error_handling(char *message);
//**********2021428080 donghyun kim***********//
int main(int argc, char *argv[]){
    int sock;
    int str_len;
    struct sockaddr_in serv_addr, clnt_addr;//server address & client(oposite side) address struct
    socklen_t clnt_addr_size;//needed at recvfrom

    if(argc != 3){
        printf("Usage : %s <IP> <Port\n", argv[0]);
        exit(1);
    }
    
    //1. sock generate
    sock = socket(PF_INET, SOCK_DGRAM, 0);
    if(sock == -1) error_handling("socket() error");
    int reuse_optval = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void*)&reuse_optval, sizeof(reuse_optval));
    //initialize and set server address struct 
    memset(&serv_addr, 0, sizeof(serv_addr));//initialize server address struct
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    while(1){
        printf("--------------------------\n");
        for(int i = 1; i < 11; i++){
            printf("%d: %s\n", i, option[i]);
        }
        printf("--------------------------\n");
        
        int num;
        while(1){    
            printf("Input option number: ");
            scanf("%d", &num);
            if(num < 1 || 10 < num){
                printf("Wrong number. type again!\n");
                continue;
            }
            else if(num == 10){
                printf("Client quit.\n");
                exit(1);
                close(sock);
            }
            break;
        }

        //send
        SO_PACKET send_packet;
        send_packet.option = num;
        str_len = sendto(sock, &send_packet, sizeof(send_packet), 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
        if(str_len == -1) error_handling("sendto() error");
        
        //receive
        SO_PACKET recv_packet;
        clnt_addr_size = sizeof(clnt_addr);
        int str_len = recvfrom(sock, &recv_packet, sizeof(recv_packet), 0, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
        if(str_len == -1) error_handling("client recvfrom() error\n");
        printf(">>> Server result: %s: value: %d, result: %d\n", option[recv_packet.option], recv_packet.optval, recv_packet.result);
    }

}


void error_handling(char* message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
