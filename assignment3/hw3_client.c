#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#define BUF_SIZE 100
#define SEQ_START 1000

typedef struct{
    int seq; //SEQ number
    int ack; //ACK number
    int buf_len; //File read/write bytes
    char buf[BUF_SIZE]; //for sending file name or file content
}Packet;

//************2021428080 donghyun kim**************//
void error_handling(char*message);

int main(int argc, char *argv[]){
    int sock;
    struct sockaddr_in serv_addr;

    if(argc != 3){
        printf("Usage: %s <IP> <Port>\n", argv[0]);
        exit(1);
    }

    //1. generate socket
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock == -1) error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    //2.connect 
    if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1){
        error_handling("connect() error\n");
        exit(1);
    }

    //3. read & write
    int fd = open("tcp.txt", O_WRONLY|O_CREAT|O_RDONLY, 0644);
    char fileName[BUF_SIZE];
    printf("Input file Name: ");
    scanf("%s", fileName);
    printf("[Client] request %s\n",fileName);
    printf("\n");

    Packet send_packet;
    strcpy(send_packet.buf, fileName);
    write(sock, &send_packet, sizeof(send_packet));

    int accum_len = 0;
    while(1){
        Packet recv_packet;
        int rx_len = 0;
        //-----receive packet-----
        //file not exsist in server
        rx_len = read(sock, &recv_packet, sizeof(recv_packet));
        if(strcmp(recv_packet.buf, "File Not Found") == 0){
            printf("%s\n", recv_packet.buf);
            exit(1);
        }
        //read error
        else if(rx_len == -1) error_handling("read() error");
        //last data, not send packet & quit
        else if(rx_len == 0 || recv_packet.buf_len < 100){//rx_len == 0 || rx_len < 100 NO!!!!! rx_len == 0 is correct but rx_len < 100 is not correct
            printf("[Client] Rx SEQ: %d, len: %d bytes\n", recv_packet.seq, recv_packet.buf_len);
            //printf("rx_len: %d, recv_pacekt.buf_len: %d, accum_len(before adding): %d\n", rx_len, recv_packet.buf_len, accum_len);
            accum_len += recv_packet.buf_len;//rx_len NO!!!!!!
            //printf("accum_len(after adding): %d\n", accum_len);
            //-----write on "tcp.txt"-----
            int final_add_len = write(fd, recv_packet.buf, recv_packet.buf_len);
            //printf("final_add_len: %d\n", final_add_len);
            //printf("%s sentence writed(recv_packet.buf)\n", recv_packet.buf);
            //total received Bytes(final accum_len)
            printf("tcp.txt received (%d Bytes)\n", accum_len);
            close(fd); close(sock);
            exit(1);
        }
        //no last data
        else{
            printf("[Client] Rx SEQ: %d, len: %d bytes\n", recv_packet.seq, recv_packet.buf_len);
            //printf("rx_len: %d, recv_pacekt.buf: %d\n",rx_len, recv_packet.buf_len);
            accum_len += recv_packet.buf_len;//rx_len NO!!!!!
            //printf("accum_len: %d\n", accum_len);
            //-----write on "tcp.txt"-----
            write(fd, recv_packet.buf, sizeof(recv_packet.buf));
            //-----send packet-----
            Packet send_packet;
            send_packet.ack = recv_packet.seq + 100;
            write(sock, &send_packet, sizeof(Packet));
            printf("[Client] Tx ACK: %d\n", send_packet.ack);
            printf("\n");
        }
    }
}

void error_handling(char*message){
    fputs(message, stderr);
    fputc('\n',stderr);
    exit(1);
}