#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>
//**********2021428080 donghyun kim***********//
#define RAND_START 1 //first random value
#define RAND_END 30//last random value

//result field value
#define COL 4 //BINGO table COL size
#define ROW 4 //BINGO table ROW size
#define FAIL 0 //fail to figure
#define SUCCESS 1 //success to figure
#define CHECKED 2 //duplicate number (already figured)

//cmd field value
#define BINGO_REQ 0
#define BINGO_RES 1
#define BINGO_END 2

//packet for client -> server
typedef struct {
    int cmd; //BINGO_REQ
    int number;//randomized number(1~30)
}REQ_PACKET;

//packet for server -> client
typedef struct {
    int cmd;//BINGO_RES or BINGO_END
    int number;//number that User sent
    int board[ROW][COL];//states for bingo check
    int result;//result from server(FAIL, SUCCESS, CHECKED)
}RES_PACKET;

void error_handling(char *message);
void print_array(int arr[ROW][COL]);

int bingo_array[ROW][COL] = {0, }; //random bingo number array(store 1 ~ 30)
int player_choice_array[ROW][COL] = {0, };//bingo state that User figured(success)

int main(int argc, char *argv[]){
    int sock;
    int str_len;
    struct sockaddr_in serv_addr, clnt_addr;//server address & client(oposite side) address struct
    socklen_t clnt_addr_size;

    if(argc != 3){
        printf("Usage : %s <IP> <Port\n", argv[0]);
        exit(1);
    }
    
    //1. sock generate
    sock = socket(PF_INET, SOCK_DGRAM, 0);
    if(sock == -1) error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    srand(time(NULL));
    while(1){
        sleep(1);
        int rand_num = (rand() % RAND_END) + RAND_START;
        printf("random Number : %d\n", rand_num);

        REQ_PACKET send_packet;
        send_packet.cmd = BINGO_REQ;
        send_packet.number = rand_num;
        
        str_len = sendto(sock, &send_packet, sizeof(send_packet), 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
        printf("[Tx] BINGO_REQ(number: %d)\n", send_packet.number);
        printf("\n");
        
        RES_PACKET recv_packet;
        int str_len = recvfrom(sock, &recv_packet, sizeof(recv_packet), 0, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
        if(str_len == -1)printf("client recvfrom is error()\n");

        printf("[Rx] BINGO_RES(number: %d, result: %d)\n", recv_packet.number, recv_packet.result);
        
        if(recv_packet.cmd == BINGO_END){
            //printf("Number : %d, SUCCESS\n",recv_packet.number);
            printf("BINGO_END!\n");
            print_array(recv_packet.board);
            exit(1);
        }
        else if(recv_packet.result == SUCCESS){
            //printf("Number : %d, SUCCESS\n",recv_packet.number);
            print_array(recv_packet.board);
        }
        else if(recv_packet.result == CHECKED){
            //printf("Number : %d, ALREADY CHECKED\n",recv_packet.number);
            print_array(recv_packet.board);
        }
        else{
            //printf("Number : %d, FAIL\n",recv_packet.number);
            print_array(recv_packet.board);
        }
        printf("\n");
    }
}

void print_array(int arr[ROW][COL]){
    printf("--------------------\n");
    for(int i = 0; i < ROW; i++){
        for(int j = 0; j < COL; j++){
            if(arr[i][j] == 0) printf("   | ");
            else printf("%d | ", arr[i][j]);
        }
        printf("\n");
        printf("--------------------\n");
    }
    printf("\n");
}


void error_handling(char* message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
