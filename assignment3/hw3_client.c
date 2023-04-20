#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h> //for open()
#define BUF_SIZE 100
#define SEQ_START 1000

typedef struct {
    int seq;
    int ack;
    int buf_len;
    char buf[BUF_SIZE];
} Packet;

void error_handling(char* message);

int main(int argc, char *argv[]){
    int sock;
    struct sockaddr_in serv_addr;//tcp client needs just one socket

    if(argc != 3){
        printf("Usage: %s <IP> <Port>\n", argv[0]);
        exit(1);
    }

    //1. generate socket
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock == -1) error_handling("socket() error\n");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    //2. connect 
    if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1){
        error_handling("connect() error\n");
    }

    //send filename to server
    char fileName[BUF_SIZE];
    printf("Input file name: ");
    scanf("%s", fileName);

    Packet name_packet;
    strcpy(name_packet.buf, fileName);
    write(sock, &name_packet, sizeof(name_packet));
    printf("[Client] request %s\n", fileName);
    printf("\n");

    //read & write
    int accum_len = 0;
    int fd = open(fileName, O_RDWR|O_CREAT, 0644);

    while(1){
        //read from server
        Packet recv_packet;
        int str_len = read(sock, &recv_packet, sizeof(recv_packet));
        
        //File not found in server
        if(strcmp(recv_packet.buf, "File Not Found") == 0) {
            printf("FIle Not Found\n");
            break;
        }
        
        //File exsist in server -> write received data to file
        printf("[Client] Rx SEQ: %d, len: %d bytes\n", recv_packet.seq, recv_packet.buf_len);
        write(fd, recv_packet.buf, recv_packet.buf_len);
        accum_len += recv_packet.buf_len;
        
        //check received data is last one
        if(recv_packet.buf_len < 100) {//last data
            printf("%s recevied (%d Bytes)\n", fileName, accum_len);
            break;
        }

        //send ACK packet to server
        Packet send_packet;
        send_packet.ack = recv_packet.seq + 100;
        write(sock, &send_packet, sizeof(send_packet));
        printf("[Client] Tx ACK: %d\n", send_packet.ack);
        printf("\n");
    } 
}

//error handling
void error_handling(char* message){
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
