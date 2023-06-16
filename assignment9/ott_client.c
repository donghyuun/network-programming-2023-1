//***********완성본*************//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <fcntl.h>
#include <time.h>

// Buffer size
#define BASIC_BUF 10
#define STANDARD_BUF 100
#define PREMIUM_BUF 1000
#define MAX_SIZE PREMIUM_BUF

// Command
#define FILE_REQ 0
#define FILE_SENDING 1
#define FILE_END 2
#define FILE_END_ACK 3

// Type
#define BASIC 1
#define STANDARD 2
#define PREMIUM 3

#define MAX_CLNT 256

typedef struct
{
    int command;
    int type;
    char buf[MAX_SIZE];
    int len;
} PACKET;

void error_handling(char *buf);
void *recv_msg(void *arg);

//2021428080 kimdonghyun
//컴파일 방법 주의: gcc ott_client.c -D_REENTRANT -o ott_client -lpthread
/*************************************************
<ott 프로그램>
- 클라이언트가 연결 요청할때마다 대응되는 쓰레드 생성
- 클라이언트가 처음 보낸 패킷에 담긴 가입자유형에 따라
  한번에 전송할 수 있는 크기 다르게함
(클라이언트)
- 서버로부터 데이터 전송받는 기능은 쓰레드로 따로 처리
**************************************************/
int main(int argc, char *argv[])
{
    int sock;
    int type, selec;
    struct sockaddr_in serv_adr;
    pthread_t recv_thread;
    void *thread_return;
    

    if(argc!=3) {
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}

    // 소켓 생성 & 초기값 설정
    sock=socket(PF_INET, SOCK_STREAM, 0);
	
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family=AF_INET;
	serv_adr.sin_addr.s_addr=inet_addr(argv[1]);
	serv_adr.sin_port=htons(atoi(argv[2]));
	
	if(connect(sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr))==-1)
		error_handling("connect() error");
	
    //선택 창
    while(1)
    {
        printf("-------------------------\n");
        printf("    K-OTT Service   \n");
        printf("-------------------------\n");
        printf(" Chose a subscribe type\n");
        printf("-------------------------\n");
        printf("1: Basic, 2: Standard, 3: Premium, 4: quit: ");
        //가입자 유형 입력받음
        scanf("%d", &type);
        if(type == 4) 
        { 
            printf("Exit program\n");
            exit(1);    
        }
        else if(type < 1 || 4 < type)
            error_handling("please select correct type number");

        printf("-------------------------\n");
        printf("1. Download, 2: Back to Main Menu: ");
        scanf("%d", &selec);
        if(selec == 2) {continue;}

        //서버로 FILE_REQ 패킷 전송
        PACKET first_packet;
        memset(&first_packet, 0, sizeof(first_packet));
        
        first_packet.command = FILE_REQ;
        first_packet.type = type; // 가입자 유형 
        write(sock,  &first_packet, sizeof(first_packet));
        break;
    }
    
	pthread_create(&recv_thread, NULL, recv_msg, (void*)&sock);
    pthread_join(recv_thread, &thread_return);//부모 쓰레드가 자식 쓰레드보다 먼저 종료되면 안되므로 기다림

    close(sock);
    return 0;
}

void *recv_msg(void *arg)
{
    int sock = *((int*)arg);
    int str_len;
    int accum_len = 0;
    struct timespec start, end;
    unsigned long t1, t2;
    const unsigned long nano = 1000000000;
    int recv_cnt = 0;
    
    clock_gettime(CLOCK_REALTIME, &start);
    t1 = start.tv_nsec + start.tv_sec * nano;
    
    //서버로부터 반복해서 데이터 읽어옴
    while(1)
    {
        PACKET recv_packet;
        memset(&recv_packet, 0, sizeof(recv_packet));
        //*************매우 중요**************
        //패킷의 속성을 다 받기 위해서는 recv_packet.buf만 읽어들이는게 아니라
        //"패킷 전체를 다 읽어야 한다!!!!"
        //만약 str_len = read(sock, &recv_packet.buf, max_size); 이렇게 사용한 경우
        //recv_packet.buf만 해당하는 값을 받게 되고,
        //recv_packet.command 등은 값을 받지 못하고 초기화햇던 값인 0으로 계속 남게 된다.
    
        // 서버로부터 데이터 읽어옴
        str_len = read(sock, &recv_packet, sizeof(recv_packet));
        recv_cnt++;

        printf("recv_packet.len: %d\n", recv_packet.len);
        if(str_len == -1)
        {
            error_handling("client thread read() error");
        }
        
        //etc) sizeof는 버퍼의 할당된 크기를, strlen는 버퍼내 문자열의 길이(널문자 제외)를 반환한다.
        accum_len = accum_len + recv_packet.len;//accum_len + strlen(recv_packet.buf) <- NO!!!
        //=> 누적합 계산이랑 읽은 것 파일에 저장할때도 그냥 패킷의 len속성을 이용해 파일에 write한다.
        //예) 과제 3 참고
        //accum_len += recv_packet.buf_len;//!!!!
        //int final_add_len = write(fd, recv_packet.buf, recv_packet.buf_len);//!!!!
        
        printf("recv_packet's command: %d\n", recv_packet.command);
        printf(".");
        
        //서버로부터의 마지막 패킷인 경우 -> 서버로 FILE_END 패킷을 전송하고 반복문 빠져나옴
        if(recv_packet.command == FILE_END)
        {   
            printf("client received FILE_END command\n");
            PACKET end_packet;
            memset(&end_packet, 0, sizeof(end_packet));
            end_packet.command = FILE_END_ACK;

            str_len = write(sock, &end_packet, sizeof(end_packet));
            if(str_len == -1) error_handling("client thread end_packet write() error");
            break;
        }

    }
    clock_gettime(CLOCK_REALTIME, &end);
    t2 = end.tv_nsec + end.tv_sec * nano;

    printf("File Transmission Finished\n");
    printf("recv_cnt: %d\n", recv_cnt);
    printf("Total received bytes: %d\n", accum_len);
    printf("Downloading time: %lu msec\n", (t2-t1)/1000000);
    printf("Client closed\n");//close at main thread

    return NULL;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}