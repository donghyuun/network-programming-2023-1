#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <fcntl.h>

#define BUF_SIZE 2048

void error_handling(char *buf);
//2021428080 kimdonghyunq
int main(int argc, char *argv[]){
    int sock;
    struct sockaddr_in serv_adr;
    struct timeval timeout;
    fd_set reads, cpy_reads;
    int fd_max = 2, fd_num;
    int chose_num;
    int str_len;
    char buf[BUF_SIZE];

    if(argc!=3) {
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}
	
	sock = socket(PF_INET, SOCK_STREAM, 0);   
	if(sock == -1)
		error_handling("socket() error");
	
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family=AF_INET;
	serv_adr.sin_addr.s_addr=inet_addr(argv[1]);
	serv_adr.sin_port=htons(atoi(argv[2]));
	
	if(connect(sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr))==-1)
		error_handling("connect() error!");
	else{        
        //below is especially essential!!! if you are receiver client
        //when using select(), fd_max(before): 2 -> fd_max(after): 3 
        //at sender client, youi get changed fd_max by making file read fd
        fd_max = (fd_max < sock) ? sock : fd_max;

        //add serv_sock to fd_set array for detection
        FD_ZERO(&reads);
        FD_SET(sock, &reads);
    }
    printf("-------------------------------\n");
	printf("   choose function:\n");
	printf("   1. sender  2. receiver\n");
	printf("-------------------------------\n");
	printf("==> ");
	scanf("%d", &chose_num);

    //sender
    if(chose_num == 1)
	{
        printf("File Sender Start!\n");
		printf("Connected.....\n");
        int file_fd;
        file_fd = open("rfc1180.txt", O_RDONLY);
        if(file_fd == -1) 
            error_handling("sender file open() error");
        fd_max = file_fd > fd_max ? file_fd : fd_max;

        printf("fd1: %d, fd2: %d\n", sock, file_fd);
        printf("max_fd: %d\n", fd_max);

        //add file fd to fd_set array for detection
        FD_SET(file_fd, &reads);
        
        while(1)
        {
            timeout.tv_sec = 3;
            timeout.tv_usec = 0;
            cpy_reads = reads;

            if((fd_num = select(fd_max + 1, &cpy_reads, 0, 0, &timeout)) == -1)
            {
                error_handling("Sender select() error");
            }
            else if(fd_num == 0)
            {
                continue;
            }
        
            for(int i = 0; i < fd_max + 1; i++)
            {
                if(FD_ISSET(i, &cpy_reads))
                {
                    //data came from server
                    memset(buf, 0, BUF_SIZE);
                    if(i == sock)
                    {
                        str_len = read(i, buf, BUF_SIZE);
                        if(str_len == -1) error_handling("Sender read() from server error");
                        else if(str_len == 0)
                        {
                            FD_CLR(i, &reads);
                            close(i);
                            printf("close client: %d\n", i);
                        }
                        else
                        {
                            printf("%s", buf); 
                        }
                    }
                    else if(i == file_fd)
                    {
                        str_len = read(i, buf, BUF_SIZE);
                        if(str_len == -1) error_handling("Sender read() from fileerror\n");
                        else if(str_len == 0)
                        {
                            FD_CLR(i, &reads);
                            close(i);
                            printf("close file; %d\n", i);
                            if(fd_max == file_fd) fd_max = sock;
                        }
                        else
                        {
                            sleep(1);
                            write(sock, buf, str_len);
                        }
                    }
                }
            }
        }
    }
    //receiver
    else if(chose_num == 2)
    {
        printf("File Receiever Start!\n");
		printf("Connected.....\n");
        printf("fd2: %d\n", sock);
        printf("max_fd: %d\n", fd_max);
        while(1)
        {
            timeout.tv_sec = 3;
            timeout.tv_usec = 0;
            cpy_reads = reads;

            if((fd_num = select(fd_max + 1, &cpy_reads, 0, 0, &timeout)) == -1)
            {
                error_handling("Receiver select() error");
            }
            else if(fd_num == 0)
            {
                continue;
            }

            for(int i = 0; i < fd_max + 1; i++)
            {
                memset(buf, 0, BUF_SIZE);
                if(FD_ISSET(i, &cpy_reads))
                {
                    if(i == sock)
                    {
                        str_len = read(i, buf, BUF_SIZE);
                        printf("%s", buf);
                        write(sock, buf, str_len);
                    }
                    else
                    {
                        printf("exception occured at annonymous fd: %d\n", i);
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