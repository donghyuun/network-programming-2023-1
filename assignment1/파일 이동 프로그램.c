//2021428080 donghyun kim
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define BUF_SIZE 10

void error_handling(char* message);

int main(int argc, char *argv[])
{
    int fd, fd2; 
    int cnt = 0, cnt_sum = 0;
    char buf[BUF_SIZE];

    //not include src/dest file
    if(argc != 3){
        printf("[Error] mymove Usage: ./mymove src_file dest_file\n");
        exit(1);
    }

    //fd for reading file
    fd = open(argv[1], O_RDONLY);
    if(fd == -1) error_handling("open() error!");
    //fd for writing file
    fd2 = open(argv[2], O_CREAT|O_WRONLY|O_TRUNC, 0644);
    if(fd2 == -1) error_handling("open() error!");
    
    //read & write in while 
    while(cnt = read(fd, buf, BUF_SIZE - 1)){// BUF_SIZE - "1" <= -1 for null charactor
        cnt_sum += cnt;//total bytes count
        write(fd2, buf, cnt);
    }
    
    remove(argv[1]);//delete src_file
    close(fd); close(fd2);
    printf("move from src.txt to dest.txt (bytes: %d) finished\n", cnt_sum);
    
    return 0;
}

void error_handling(char* message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}