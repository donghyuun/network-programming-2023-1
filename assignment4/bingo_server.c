#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdlib.h>
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

void error_handling(char* message);
int check_array(int arr[ROW][COL], int num);
void print_array(int arr[ROW][COL]);
int check_full(int arr[ROW][COL]);
//void copy_array(int arr1[][], int arr2[][]);
int bingo_array[ROW][COL] = {0, }; //random bingo number array(store 1 ~ 30)
int player_choice_array[ROW][COL] = {0, };//bingo state that User figured(success)
int x, y;

int main(int argc, char *argv[]){
    int sock;
    int str_len;
    struct sockaddr_in serv_addr, clnt_addr;//server & client address struct 
    socklen_t clnt_addr_size;//size struct of client address struct
    if(argc != 2){
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    //make randomized bingo table (4x4)
    srand(time(NULL));//initialize random seed to current time value
    for(int i = 0; i < ROW; i++){
        for(int j =0; j < COL; j++){
            while(1){
                int isDup = 0;
                int rand_num = (rand() % RAND_END) + RAND_START;//1~30
                for(int k = 0; k < ROW; k++){
                    for(int h = 0; h < COL; h++){
                        if((k != i || h != j) && bingo_array[k][h] == rand_num)
                            isDup = 1;
                    }
                }
                if(!isDup) {
                    bingo_array[i][j] = rand_num;
                    break;
                } 
            }
        }
    }
    print_array(bingo_array);

    //1. socket generate
    sock = socket(PF_INET, SOCK_DGRAM, 0);
    if(sock == -1)
        error_handling("socket() error");
    //set server address struct 
    memset(&serv_addr, 0, sizeof(serv_addr));//intialize server address socket
    serv_addr.sin_family = AF_INET;//IPv4
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);//server IP address
    serv_addr.sin_port = htons(atoi(argv[1]));//server Port number

    //2. bind   
    if(bind(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error");
    
    //run
    while(1)
    {   
        REQ_PACKET recv_packet;
        str_len = recvfrom(sock, &recv_packet, sizeof(recv_packet), 0, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
        if(str_len == -1) {
            printf("recvfrom() error");
            break;
        }
        printf("[Rx] BINGO_REQ(cmd: %d ,number: %d\n", recv_packet.cmd, recv_packet.number);

        int recv_num = recv_packet.number;
        RES_PACKET send_packet;
        if(check_array(bingo_array, recv_num)){//number is in bingo_array
            if(check_array(player_choice_array, recv_num)){//already checked
               send_packet.cmd = BINGO_RES;
               send_packet.number = recv_num;
               send_packet.result = CHECKED;
               for(int i = 0; i <ROW; i++){
                    for(int j = 0; j < COL; j++){
                        send_packet.board[i][j] = player_choice_array[i][j];
                    }
                }
               
               sendto(sock, &send_packet, sizeof(send_packet), 0, (struct sockaddr*)&clnt_addr, sizeof(clnt_addr));
               printf("Number : %d, ALREADY CHECKED\n",send_packet.number);
               printf("[Tx] BINGO_RES(cmd: %d, result: %d)\n", send_packet.cmd, send_packet.result);
               print_array(player_choice_array);
               continue;
            }
            //success
            for(int i = 0; i< ROW; i++){//insert num to player_choice_array
                for(int j = 0; j <COL; j++){
                    if(bingo_array[i][j] == recv_num) player_choice_array[i][j] = recv_num;
                }
            }
            if(check_full(player_choice_array)){//all success
                send_packet.cmd = BINGO_END;
                send_packet.number = recv_num;
                send_packet.result = SUCCESS;
                for(int i = 0; i <ROW; i++){
                    for(int j = 0; j < COL; j++){
                        send_packet.board[i][j] = player_choice_array[i][j];
                    }
                }

                sendto(sock, &send_packet, sizeof(send_packet), 0, (struct sockaddr*)&clnt_addr, sizeof(clnt_addr));
                printf("bingo: [%d][%d] number: %d\n", x, y, recv_num);
                printf("[Tx] BINGO_RES(cmd: %d, result: %d)\n", send_packet.cmd, send_packet.result);
                printf("No available space\n");
                printf("BINGO_END!\n");
                print_array(player_choice_array);
                exit(1);
            }
            else{//normal success
                send_packet.cmd = BINGO_RES;
                send_packet.number = recv_num;
                send_packet.result = SUCCESS;
                for(int i = 0; i <ROW; i++){
                    for(int j = 0; j < COL; j++){
                        send_packet.board[i][j] = player_choice_array[i][j];
                    }
                }
                    
                sendto(sock, &send_packet, sizeof(send_packet), 0, (struct sockaddr*)&clnt_addr, sizeof(clnt_addr));
                printf("bingo: [%d][%d] number: %d\n", x, y, recv_num);
                printf("[Tx] BINGO_RES(cmd: %d, result: %d)\n", send_packet.cmd, send_packet.result);
                print_array(player_choice_array);
            }
        }
        else{//fail
            send_packet.cmd = BINGO_RES;
            send_packet.number = recv_num;
            send_packet.result = FAIL;
            for(int i = 0; i <ROW; i++){
                for(int j = 0; j < COL; j++){
                    send_packet.board[i][j] = player_choice_array[i][j];
                }
            }
            
            sendto(sock, &send_packet, sizeof(send_packet), 0, (struct sockaddr*)&clnt_addr, sizeof(clnt_addr));
            printf("Number : %d, FAIL\n",send_packet.number);
            printf("[Tx] BINGO_RES(cmd: %d, result: %d)\n", send_packet.cmd, send_packet.result);
            print_array(player_choice_array);
        }

    }


    close(sock);
    return 0;
}

void error_handling(char* message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

int check_array(int arr[ROW][COL], int num){
    for(int i = 0; i <ROW; i++){
        for(int j = 0; j < COL; j++){
            if(arr[i][j] == num) {
                x= i; y = j;
                return 1;//exsits
            }
        }
    }
    return 0;//not exsist
}

int check_full(int arr[ROW][COL]){
    for(int i = 0; i <ROW; i++){
        for(int j = 0; j < COL; j++){
            if(arr[i][j] == 0) {
                return 0;
            }
        }
    }
    return 1;
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

/*
void copy_array(int arr1[ROW][COL], int arr2[ROW][COL]){
    for(int i = 0; i <ROW; i++){
        for(int j = 0; j < COL; j++){
            arr1[i][j] = arr2[i][j];
        }
    }
}
*/
