#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>
#include <signal.h>

#define TTL 64
#define MSG_BUF_SIZE 150
#define INPUT_BUF_SIZE 100
void error_handling(char *message);

int main(int argc, char *argv[]){
    int recv_sock, send_sock;//recv_sock -> child, send_sock -> parent
	int str_len;
    int time_live = TTL;
	char buf[MSG_BUF_SIZE];
	struct sockaddr_in mul_adr, adr;//mul_adr -> sender, adr -> receiver
    struct ip_mreq join_adr;//to join in multicast group

    if(argc != 4){
		printf("Usage : %s <GroupIP> <PORT> <NAME>\n", argv[0]);
		exit(1);
	} 
    
    int pid  = fork();
    if(pid == -1)
        error_handling("fork() error");

    //child process, for receiving data
    if(pid == 0)
    {
        //recv_sock is like serv_sock
        recv_sock = socket(PF_INET, SOCK_DGRAM, 0);
        join_adr.imr_multiaddr.s_addr=inet_addr(argv[1]);//multicast group address to join 
	    join_adr.imr_interface.s_addr=htonl(INADDR_ANY);//my ip address
        
        int optval = 1;
        int state;
        //join multicast group, this should be done before bind()!!!
        state = setsockopt(recv_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*)&join_adr, sizeof(join_adr));
        if(state == -1) error_handling("[child] ADD_MEMBERSHIP setsockopt() error");
        state = setsockopt(recv_sock, SOL_SOCKET, SO_REUSEADDR, (void*)&optval, sizeof(optval));
        if(state == -1) error_handling("[child] REUSEADDR setsockopt() error");
        
 	    memset(&adr, 0, sizeof(adr));
    	adr.sin_family = AF_INET;
	    adr.sin_addr.s_addr = htonl(INADDR_ANY);	
	    adr.sin_port = htons(atoi(argv[2]));

        if(bind(recv_sock, (struct sockaddr*) &adr, sizeof(adr)) == -1) 
        { 
            error_handling("bind() error"); 
        }

        while(1)
        {
            memset(&buf, 0, sizeof(buf));
            str_len = recvfrom(recv_sock, buf, MSG_BUF_SIZE-1, 0, NULL, 0);
            if(str_len == -1)
                error_handling("[child] recvfrom() error");
            buf[str_len] = 0;
            //print buf on monitor
            fputs(buf, stdout);
        }
        close(recv_sock);
        return 0;

    }
    //parent process, for sending data
    else
    {   
        send_sock = socket(PF_INET, SOCK_DGRAM, 0);
	    memset(&mul_adr, 0, sizeof(mul_adr));
	    mul_adr.sin_family=AF_INET;
	    mul_adr.sin_addr.s_addr=inet_addr(argv[1]);  // Multicast IP
	    mul_adr.sin_port=htons(atoi(argv[2]));       // Multicast Port
        setsockopt(send_sock, IPPROTO_IP, IP_MULTICAST_TTL, (void*)&time_live, sizeof(time_live));
        
        while(1)
        {
            //memset(&buf, 0, sizeof(buf));
            fgets(buf, INPUT_BUF_SIZE, stdin);//do not use 0!!!, use "stdin"!!!!!
            
            //'q\n' or 'Q\n' -> terminate child & parent preocess
            if(strcmp(buf,"q\n")  == 0 || strcmp(buf,"Q\n") == 0)
            {
                kill(pid, SIGKILL);
                printf("SIGKILL: Multicast Receiver terminate!\n");
                printf("Multicast Sender(Parent process) exit!\n");
                break;
            }

            //message concat, to be "[NAME] MESSAGE"
            char msg[MSG_BUF_SIZE] = "";
            strcat(msg, "["); strcat(msg, argv[3]); strcat(msg, "]"); 
            strcat(msg, " ");
            strcat(msg, buf);

            sendto(send_sock, msg, strlen(msg), 0, (struct sockaddr*)&mul_adr, sizeof(mul_adr));
        }

        close(send_sock);
        return 0;
    }
}

void error_handling(char* message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}