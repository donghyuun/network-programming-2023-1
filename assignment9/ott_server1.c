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

#define MAX_CLNT 256

#define FILE_REQ 0
#define FILE_SENDING 1
#define FILE_END 2
#define FILE_END_ACK 3
#define FILE_ERROR -1

// Subscriber type
#define BASIC 1
#define STANDARD 2
#define PREMIUM 3

// Buffer size
#define BASIC_BUF 10
#define STANDARD_BUF 100
#define PREMIUM_BUF 1000

#define MAX_SIZE PREMIUM_BUF

int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];

pthread_mutex_t mutex1;
pthread_mutex_t mutex2;

typedef struct
{
    int command;
    int type;
    char buf[MAX_SIZE];
    int len;
} PACKET;

void error_handling(char *msg);
void send_file(int client, int type);
void get_user_type(int type, char *user_type);

void* handle_clnt(void *arg);
int handle_rcvmsg(int clnt_sock, PACKET info);
void display_pakcet(char *dir, PACKET *packet);

/*************************************************
<ott 프로그램>
- 클라이언트가 연결 요청할때마다 대응되는 쓰레드 생성
- 클라이언트가 처음 보낸 패킷에 담긴 가입자유형에 따라
  한번에 전송할 수 있는 크기 다르게함
(서버)
- 서버(메인쓰레드)엔 클라이언트 연결 요청만 처리하고 
- 클라이언트와 통신하는 기능은 쓰레드로 따로 처리
**************************************************/
int main(int argc, char *argv[])
{
    FILE *fp;

    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    int clnt_adr_sz;
    pthread_t t_id;

    if (argc != 2)
    {
        printf("Usage : %s <port\n", argv[0]);
        exit(1);
    }

    printf("---------------------\n");
    printf("K-OTT Service Server\n");
    printf("---------------------\n");

    //락 변수 초기값 설정
    pthread_mutex_init(&mutex1, NULL);
    pthread_mutex_init(&mutex2, NULL);

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);

    //서버 주소 구조체 초기값 설정
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("bind() error");
    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    while (1)
    {   
        // 반복문으로 다수의 클라이언트 요청 처리 
        clnt_adr_sz = sizeof(clnt_adr);
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);

        // 공유 변수 접근 -> mutext_lock(mutex1) 사용, clnt_sock 삭제는 나중에 처리
        pthread_mutex_lock(&mutex1);
        clnt_socks[clnt_cnt++] = clnt_sock;//연결중인 클라이언트 정보 담은 배열에 추가
        pthread_mutex_unlock(&mutex1);

        // 자식 쓰레드 생성
        pthread_create(&t_id, NULL, handle_clnt, (void *)&clnt_sock);
        pthread_detach(t_id);
        printf("Connceted client IP: %s, clnt_sock=%d\n", inet_ntoa(clnt_adr.sin_addr), clnt_sock);
    }

    // 서버 소켓 닫음
    close(serv_sock);
    // mutex 락 지움
    pthread_mutex_destroy(&mutex1);
    pthread_mutex_destroy(&mutex2);

    return 0;
}

// 쓰레드가 수행하는 함수
void* handle_clnt(void *arg)
{
    int clnt_sock = *((int *)arg);//클라이언트와 연결된 소켓 번호(파일 디스크립터 값)
    int str_len = 0, i;
    int accum_len = 0;//<- 테스트용 끝나면 지우면됨
    PACKET recv_packet;

    memset(&recv_packet, 0, sizeof(PACKET));
    
    // FILE_REQ 패킷 수신
    
    str_len = read(clnt_sock, &recv_packet, sizeof(PACKET));
    if(str_len == -1) error_handling("thread: read() error");
    else if(recv_packet.command != FILE_REQ) error_handling("packet is not FILE_REQ pakcet");
    
    int my_type = recv_packet.type;
    int buf_size = 0;
    if (my_type == BASIC)
        buf_size = BASIC_BUF;
    else if (my_type == STANDARD)
        buf_size = STANDARD_BUF;
    else if (my_type == PREMIUM)
        buf_size = PREMIUM_BUF;
    else
        buf_size = BASIC_BUF;
    
    int fd = open("hw09.mp4", O_RDONLY);
    if(fd == -1) error_handling("thread: file open error()");

    // 파일 읽어서 클라이언트로 전송
    while(1)
    {
        PACKET send_packet;
        str_len = read(fd, send_packet.buf, buf_size);
        if(str_len == -1) error_handling("file read error()");
        else if(str_len == 0) break;
        //-----mutex lock-----//
        pthread_mutex_lock(&mutex1);
        send_packet.type = my_type;
        send_packet.command = FILE_SENDING;
        send_packet.len = str_len;

        accum_len += str_len;
        write(clnt_sock, (void *)&send_packet, sizeof(PACKET));
        //-----mutex unlock-----//
        pthread_mutex_unlock(&mutex1);
    }
    close(fd);
    
    // FILE_END 패킷 전송
    PACKET end_packet;
    end_packet.type = my_type;
    end_packet.command = FILE_END;
    end_packet.len = 0;

    pthread_mutex_lock(&mutex1);
    write(clnt_sock, (void *)&end_packet, sizeof(PACKET));
    pthread_mutex_unlock(&mutex1);

    printf("\nTotal Tx Bytes: %d to Client %d\n", accum_len, clnt_sock);

    //FILE_END_ACK 패킷 수신
    PACKET recv_packet2;
    str_len = read(clnt_sock, &recv_packet2, sizeof(recv_packet));
    if(str_len == -1) error_handling("thread: FILE_END_ACK packet read() error");
    else if(recv_packet2.command != FILE_END_ACK) error_handling("thread: this packet is not FILE_END_ACK packet");
    
    // 공유변수 접근 -> mutext_lock(mutex2) 사용
    pthread_mutex_lock(&mutex2);
    // 연결 끊긴 클라이언트 삭제
    for (i = 0; i < clnt_cnt; i++) 
    {
        if (clnt_sock == clnt_socks[i])
        {
            printf("clnt_sock: %d closed\n", clnt_sock);
            while (i < clnt_cnt)
            {
                clnt_socks[i] = clnt_socks[i + 1];
                i++;
            }
            break;
        }
    }
    clnt_cnt--;
    // mutex_unlock(mutex2)
    pthread_mutex_unlock(&mutex2);
    close(clnt_sock);
    return NULL;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}