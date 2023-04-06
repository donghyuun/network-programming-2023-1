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

void error_handling(char *message);
int main(int argc, char* argv[]){
    int serv_sock;//listening socket 
    int clnt_sock;//data trasferring socket
    //char message[BUF_SIZE];
    //int str_len, i;
    struct sockaddr_in serv_addr;//for server information
    struct sockaddr_in clnt_addr;//for client information
    socklen_t clnt_addr_size;

    if(argc != 2){
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    //1. generate
    serv_sock=socket(PF_INET, SOCK_STREAM, 0);
    if(serv_sock == -1) error_handling("socket() error");
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    //2. bind
    if(bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == 1)
        error_handling("bind() error");
    else{
        printf("------------------------\n");
        printf("File Transmission Server\n");
        printf("------------------------\n");
    }
    //3. listen
    if(listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    //4. accept
    clnt_addr_size = sizeof(clnt_addr);;
    clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
    if(clnt_sock == -1) error_handling("accept() error");

    //5. read & write 
    Packet name_packet;
    read(clnt_sock, &name_packet, sizeof(Packet));
    char *fileName = name_packet.buf;//filename;
    
    //check file is existing by open Method
    int fd = open(fileName, O_RDONLY);
    //file does not exsist, send error message to client & quit
    if(fd == -1) {
        Packet send_packet;
        char *message = "File Not Found";
        strcpy(send_packet.buf, message);
        printf("a.txt File Not Found\n");
        //send error message 
        write(clnt_sock, &send_packet, sizeof(Packet));
        exit(1);    
    }
    
    //file exsist
    printf("[Server] sending tcp.txt\n");
    printf("\n");
    int start_num = SEQ_START;
    int accum_len = 0;
    while(1){
        Packet recv_packet;
        Packet send_packet;
        int rx_len = 0;//read bytes per one reading
        //read data from file  cf) sizeof(send_packet.buf) -1 NONONO!!!!!!!!!!
        rx_len = read(fd, send_packet.buf, sizeof(send_packet.buf));
        accum_len += rx_len;

        //error occure
        if(rx_len == -1) {
            error_handling("read() error");
            break;
        }
        else if(rx_len == 0){

            break;
        }
        //no error
        else{
            send_packet.buf_len = rx_len;
            send_packet.seq = start_num;
            
            //send packet(with SEQ, Data) 
            write(clnt_sock, &send_packet, sizeof(Packet));
            printf("[Server] Tx: SEQ: %d, %d byte data\n", start_num, rx_len);
            if(rx_len < BUF_SIZE){
                printf("tcp.txt sent (%d Bytes)\n", accum_len);
                break;
            }
            //receive packet(with ACK) 
            read(clnt_sock, &recv_packet, sizeof(Packet));
            start_num = recv_packet.ack;
            printf("[Server] Rx ACK: %d\n", recv_packet.ack);
            printf("\n");
        }

    }
    
    return 0;

}

void error_handling(char*message){
    fputs(message, stderr);
    fputc('\n',stderr);
    exit(1);
}