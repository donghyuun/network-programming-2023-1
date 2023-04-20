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

void error_handling(char* message);

typedef struct {
    int cmd; //0: request, 1: response, 2: quit
    char addr[20]; // dotted-decimal address (20byte)
    struct in_addr iaddr;// result of inet_aton() 
    int result;//0: Error, 1: Success
} Packet;

int main(int argc, char *argv[]){
    
    int sock; //tcp client needs just one socket
    struct sockaddr_in serv_addr;

    if(argc != 3){
        printf("Usage: %s <IP> <Port>\n", argv[0]);
        exit(1);
    }
    
    //1. generate socket
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock == -1) {   
        error_handling("socket() error\n");
        return 0;   
    }

    //initialize & set serv_addr
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;//ipv4
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);//server IP adress
    serv_addr.sin_port = htons(atoi(argv[2]));//server Port number

    //2. connect 
    if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1){
        error_handling("connet() error\n");
        exit(1);
    }

    while(1){
        //generate packet & set data in packet
        Packet send_packet;

        printf("Input dotted-decimal address: ");
        scanf("%s", send_packet.addr);

        //-----send packet-----
        //quit
        if(strcmp(send_packet.addr, "quit") == 0){
            send_packet.cmd = QUIT;
            int wr_len = write(sock, &send_packet, sizeof(send_packet));
            printf("[Tx] cmd: %d(QUIT)\n", send_packet.cmd);
            printf("Client socket close and exit\n");
            break;
        }
        //not quit
        send_packet.cmd = REQUEST;
        printf("[Tx] cmd: %d, address: %s\n", send_packet.cmd, send_packet.addr);
        int wr_len = write(sock, &send_packet, sizeof(send_packet));

        //-----receive packet-----
        Packet recv_packet;
        int rd_len = read(sock, &recv_packet, sizeof(recv_packet));
        //read error
        if(rd_len == -1) {
            printf("read() error\n");
            break;
        }
        //convert fail
        if(recv_packet.result == ERROR){
            printf("[Rx] cmd: %d, address conversion fail! (result: %d)\n", recv_packet.cmd, recv_packet.result);
        } 
        //convert success
        else{
            printf("[Rx] cmd: %d, address conversion: %#x\n", recv_packet.cmd, recv_packet.iaddr.s_addr);
        }
        printf("\n");
    }
}

//error handling
void error_handling(char* message){
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
