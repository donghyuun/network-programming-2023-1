#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

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
    if(serv_sock == 1) error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    //2. bind
    if(bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1){
        error_handling("bind() error");
    }else{
        printf("------------------------\n");
        printf("File Transmission Server\n");
        printf("------------------------\n");
    }

    //3. listen
    if(listen(serv_sock, 5) == -1) error_handling("listen() error");

    //4. accept
    clnt_addr_size = sizeof(clnt_addr);
    clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);

    //receive file name
    Packet name_packet; 
    read(clnt_sock, &name_packet, sizeof(name_packet));
    printf("Received filename: %s\n", name_packet.buf);
    char fileName[BUF_SIZE]; strcpy(fileName, name_packet.buf);//copy filename

    //open file
    int fd = open(fileName, O_RDONLY);
    if(fd == -1) {//file not exsist
        printf("%s File Not Found\n", fileName);
        Packet send_packet;
        strcpy(send_packet.buf, "File Not Found");
        write(clnt_sock, &send_packet, sizeof(send_packet));
        exit(1);
    }else{//file exsist
        printf("[Server] sending %s\n", fileName);
        printf("\n");
    }
    //run
    int accum_len = 0;
    int current_seq = SEQ_START;
    while(1){
        //1. read from file & copy to send_packet
        Packet send_packet;
        int str_len  = read(fd, &send_packet.buf, sizeof(send_packet.buf));
        
        if(str_len == -1 ) printf("read() error\n");//read error
 
        //printf("copied bytes size: %d\n", str_len);
        send_packet.buf_len = str_len; 
        accum_len += str_len;
        send_packet.seq = current_seq; 

        //2. send packet to client
        write(clnt_sock, &send_packet, sizeof(send_packet));
        printf("[Server] Tx: SEQ: %d, %d byte data\n",send_packet.seq, send_packet.buf_len);
        
        //when last read
        if(send_packet.buf_len < 100) {
            printf("%s sent (%d bytes)\n", fileName, accum_len);
            break;
        }
        
        //3. receive packet from client
        Packet recv_packet;
        read(clnt_sock, &recv_packet, sizeof(recv_packet));
        printf("[Server] Rx ACK: %d\n", recv_packet.ack);
        printf("\n");
        current_seq = recv_packet.ack;
    }

    close(fd);
    close(serv_sock);
    close(clnt_sock);
    return 0;
}

//error handling
void error_handling(char* message){
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
