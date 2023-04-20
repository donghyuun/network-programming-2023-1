#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
//cmd
#define REQUEST 0
#define RESPONSE 1
#define QUIT 2
//redsult
#define ERROR 0
#define SUCCESS 1

void error_handling(char *message);

typedef struct {
    int cmd; //0: request, 1: response, 2: quit
    char addr[20]; // dotted-decimal address (20byte)
    struct in_addr iaddr;// result of conversion by inet_aton() 
    int result;//0: error, 1: success
}Packet;

int main(int argc, char *argv[]){
    int serv_sock;
    int clnt_sock;

    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;

    if(argc != 2){
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    //1. socket generate
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if(serv_sock == -1) error_handling("socket() error\n");
    //initialize & set serv_addr
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;//ipv4
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);//server IP adress
    serv_addr.sin_port = htons(atoi(argv[1]));//server Port number

    //2. bind
    if(bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error\n");
    else{
        printf("------------------------\n");
        printf("File Transmission Server\n");
        printf("------------------------\n");
    }
    //3. listen
    if(listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    //4. accept
    clnt_addr_size = sizeof(clnt_addr_size);//important!
    clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
    if(clnt_sock == -1) error_handling("accpet() error\n");

    //5. read & write
    while(1){
        //----read packet----
        Packet recv_packet;
        int rd_len = read(clnt_sock, &recv_packet, sizeof(recv_packet));
    
        //----send_packet----
        Packet send_packet;
        send_packet.cmd = RESPONSE;
        //quit
        if(recv_packet.cmd == QUIT){
            printf("[Rx] QUIT Message received\n");
            printf("Server socket close and exit\n");
            break;
        }
        //not quit
        printf("[Rx] Received Dotted-Decimal Address: : %s\n", recv_packet.addr);
        
        //conversion fail
        if(inet_aton(recv_packet.addr, &send_packet.iaddr) == 0){//convert addr to iaddr and store it to the right parameter
            send_packet.result = ERROR;
            printf("[Tx] Address conversion fail: %s\n", recv_packet.addr);
            write(clnt_sock, &send_packet, sizeof(send_packet));
            printf("\n");
            continue;
        }
        //conversion success
        else{
            send_packet.result = SUCCESS;
            printf("inet_aton(%s) -> %#x\n", recv_packet.addr, send_packet.iaddr.s_addr);
            printf("[Tx] cmd: %d, iaddr: %#x, result: %d\n", send_packet.cmd, send_packet.iaddr.s_addr, send_packet.result);
        }
        int wr_len = write(clnt_sock, &send_packet, sizeof(send_packet));
        printf("\n");
    }
    
}

void error_handling(char*message){
    fputs(message, stderr);
    fputc('\n',stderr);
    exit(1);
}
