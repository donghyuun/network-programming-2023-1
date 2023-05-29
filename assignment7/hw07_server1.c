#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>

#define BUF_SIZE 2048

void error_handling(char *buf);
//2021428080 kimdonghyun
int main(int argc, char*argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    struct timeval timeout;
    fd_set reads, cpy_reads;
    int clnt1, clnt2, clnt_count = 0;

    socklen_t adr_sz;
    int fd_max, str_len, fd_num;
    char buf[BUF_SIZE];

    if(argc != 2){
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    //server socket generation
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    //bind
    if(bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("bind() error");
    //listen
    if(listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    FD_ZERO(&reads);//initialize fd_set array
    FD_SET(serv_sock, &reads);//add to fd_set array for detection
    fd_max = serv_sock;//current max fd 
    printf("fd_max = %d\n", fd_max);

    while(1)
    {
        cpy_reads = reads;
        timeout.tv_sec = 3;
        timeout.tv_usec = 0;

        if((fd_num = select(fd_max+1, &cpy_reads, 0,0, &timeout)) == -1)
            break;
        if(fd_num == 0)
            continue;
        //if there are some data(exception) in fd 
        for(int i = 0; i < fd_max+1; i++)
        {
            if(FD_ISSET(i, &cpy_reads))
            {
                //if i is serv_sock => connection request
                if(i == serv_sock)
                {
                    adr_sz = sizeof(clnt_adr);
                    clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);
                    printf("client connected: %d\n", clnt_sock);
                    if(clnt_count  == 0) {
                        clnt1 = clnt_sock;
                        clnt_count++;
                    }
                    else clnt2 = clnt_sock;
                    //add this clnt sock to fd_set array for detection
                    FD_SET(clnt_sock, &reads);
                    if(fd_max < clnt_sock)
                        fd_max = clnt_sock;
                    printf("fd_max: %d\n", fd_max);
                }
                //if i is clnt_sock => receive data
                else 
                {
                    str_len = read(i, buf, BUF_SIZE);
                    if(str_len == 0)
                    {
                        FD_CLR(i, &reads);
                        close(i);
                        printf("closed client: %d\n", i);
                    }
                    //if data came from client1 -> send received data to client2
                    //if data came from client2 -> send received data to client1
                    else
                    {   
                        if(i == clnt1) 
                        {
                            write(clnt2, buf, str_len);
                            printf("data came from %d, send to %d\n", i, clnt2);
                        }
                        else if(i == clnt2)
                        {
                            write(clnt1, buf, str_len);
                            printf("data came from %d, send to %d\n", i, clnt1);
                        }
                    }
                }

            }
        }
    }

}

void error_handling(char *buf)
{
    fputs(buf, stderr);
    fputc('\n', stderr);
    exit(1);
}