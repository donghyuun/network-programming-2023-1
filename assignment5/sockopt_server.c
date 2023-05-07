#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
//option name
char *option_name[11] = {
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
//optoin value
int option_value[10] = {
    0, 
    SO_SNDBUF, 
    SO_RCVBUF, 
    SO_REUSEADDR, 
    SO_KEEPALIVE,
    SO_BROADCAST,
    IP_TOS,
    IP_TTL,
    TCP_NODELAY,
    TCP_MAXSEG,
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
    int udp_sock, tcp_sock;
    int str_len;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t optlen = 0, clnt_addr_size = 0;

    if(argc != 2){
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    //1. generate socket
    udp_sock = socket(PF_INET, SOCK_DGRAM, 0);
    tcp_sock = socket(PF_INET, SOCK_STREAM, 0);
    if(udp_sock == -1 || tcp_sock == -1) error_handling("socket() error");
    //intialize & set server address struct
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;//Ipv4
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);//IP address
    serv_addr.sin_port = htons(atoi(argv[1]));//port number
    
    //2. bind
    if(bind(udp_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1){
        error_handling("bind() error");
    }

    printf("Socket Option Server Start\n");
    //run
    while(1){
        //receive recv_packet
        SO_PACKET recv_packet;
        clnt_addr_size = sizeof(clnt_addr);//very important
        str_len = recvfrom(udp_sock, &recv_packet, sizeof(recv_packet), 0, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
        if(str_len == -1) error_handling("recvfrom() error");
        printf(">>> Received Socket option: %s\n", option_name[recv_packet.option]);
        
        //set send_packet
        SO_PACKET send_packet;
        //set send_packet's level
        int rcv_option = recv_packet.option;
        if(1 <= rcv_option && rcv_option <= 5){
            send_packet.level = SOL_SOCKET;
        } else if(6 <= rcv_option && rcv_option <= 7){
            send_packet.level = IPPROTO_IP;
        } else if(8 <= rcv_option && rcv_option <= 9){
            send_packet.level = IPPROTO_TCP;
        } 
        //set send_packet's option
        send_packet.option = recv_packet.option;
        //set send_packet's optval
        optlen = sizeof(send_packet.optval);
        int state = getsockopt(tcp_sock, send_packet.level, option_value[send_packet.option], &send_packet.optval, &optlen);
        if(state) error_handling("getsockopt() error");
        //set send_packet's result
        send_packet.result = 0;
        
        //send_packet
        str_len = sendto(udp_sock, &send_packet, sizeof(send_packet), 0, (struct sockaddr*)&clnt_addr, sizeof(clnt_addr));
        if(str_len == -1) error_handling("sendto() error");
        printf("<<< Send option: %s: %d, result: %d\n", option_name[send_packet.option], send_packet.optval, send_packet.result);
        printf("\n");    
    }
}

void error_handling(char* message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}