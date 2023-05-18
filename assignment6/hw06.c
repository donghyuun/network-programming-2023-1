#pragma warning(disable : 4996)
#define _XOPEN_SOURCE 200 // for struct sigaction

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>

void child_timeout(int sig);
void parent_timeout(int sig);
void child_terminated(int sig);
void parent_quit(int sig);

int time = 0;
int count = 0;

//2021428080 KIM DONGHYUN
int main(int argc, char *argv[])
{

    pid_t pid;//typedef int pid_t

    //----- make a new process -----
    pid = fork();

    // child process
    if (pid == 0)
    {
        //----- enroll sig handler to sig in Child Process -----
        struct sigaction alrm_act;

        alrm_act.sa_handler = child_timeout;
        sigemptyset(&alrm_act.sa_mask);
        alrm_act.sa_flags = 0;

        sigaction(SIGALRM, &alrm_act, 0);

        printf("Child process created\n");

        for (int i = 0; i < 5; i++)
        {
            alarm(5);
            sleep(10); // wake up when call SIGALRM
        }

        return 5;
    }
    // parent process
    else
    {
        //----- enroll sig handler to sig in Parent process -----
        // SIGALRM
        struct sigaction alrm_act;
        alrm_act.sa_handler = parent_timeout;
        sigemptyset(&alrm_act.sa_mask);
        alrm_act.sa_flags = 0;
        sigaction(SIGALRM, &alrm_act, 0);

        // SIGCHLD
        struct sigaction chld_act;
        chld_act.sa_handler = child_terminated;
        sigemptyset(&chld_act.sa_mask);
        chld_act.sa_flags = 0;
        sigaction(SIGCHLD, &chld_act, 0);

        // SIGINT
        struct sigaction quit_act;
        quit_act.sa_handler = parent_quit;
        sigemptyset(&quit_act.sa_mask);
        quit_act.sa_flags = 0;
        sigaction(SIGINT, &quit_act, 0);

        printf("Parent process created\n");

        alarm(2);

        // to receive
        while (1)
        {
            sleep(1);
        }
    }
}


void child_timeout(int sig)
{
    // SIGALRM from chld process
    count += 1; // count of timeout function called
    time += 5;  // timeout function called per 5 sec

    // child_time starts at 5, child_count starts at 1
    printf("[Child] Time out: 5, elapssed time: %d seconds(%d)\n", time, count);
}

void parent_timeout(int sig)
{
    // count variable isn't needed(because this loop is infinite)

    // SIGALRM from chld process
    time += 2; // timeout function called per 5 sec
    // child_time starts at 5, child_count starts at 1
    printf("<Parent> Time out: 2, elapsed time: %d seconds\n", time);
    // alarm per 5 secs, until child_count == 5
    alarm(2);
}

void child_terminated(int sig)
{
    int status;
    pid_t id = waitpid(-1, &status, WNOHANG);
    if (WIFEXITED(status)) // if normal terminate
    {
        printf("Child id = %d, sent: %d\n", id, WEXITSTATUS(status));
    }
}

void parent_quit(int sig)
{
    printf("Do you want to exit (y or Y to exit)? ");
    char ch;
    scanf("%c", &ch);
    printf("\n");
    if(ch == 'y' || ch == 'Y') exit(1);
}