//Ben Levi 318811304

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/fcntl.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <linux/random.h>
#include <stdbool.h>


#define SIZE 10

char* ERROR_MSG =  "ERROR_FROM_EX4\n";
char* SRV_FILE = "to_srv.txt";
int TIMEOUT_SEC = 30;
char* TIMEOUT_MSG = "Client closed because no response was received from the server for 30 seconds\n";


void printError() {
    printf("%s", ERROR_MSG);
}
void errorHandle() {
    printError();
    exit(-1);
}
void errorHandleWithClose(int fd) {
    close(fd);
    printError();
    exit(-1);
}
void default_hand(int sig) {
    signal(SIGUSR1, default_hand);
}

// Generate random number between 0-5.
int generateNum() {
    int s;
    if(syscall(SYS_getrandom, &s, sizeof(int), 0) == -1) {
        errorHandle();
    }
    if(s < 0) {
        s *= (-1);
    }
    return s%6;
}

void writeParams(int fdOut, char* params[]) {
    // Linux pid is max 5 digits.
    char pid[6] = {0};
    sprintf(pid, "%d", getpid());
    if(write(fdOut, pid, strlen(pid)) == -1) {
        errorHandleWithClose(fdOut);
    }
    int i = 2;
    while (i < 5) {
        if(write(fdOut, "\n", 1) == -1) {
            errorHandleWithClose(fdOut);
        }
        if(write(fdOut, params[i], strlen(params[i])) == -1) {
            errorHandleWithClose(fdOut);
        }
        i++;
    }
    if(close(fdOut) == -1) {
        errorHandle();
    }
}


void signal_hand(int sig) {
    alarm(0);
    //signal(SIGUSR1, signal_hand);

    int i;
    char pid[6] = {0};
    sprintf(pid, "%d", getpid());
    int lim = (int)strlen(pid) + 15;
    char clientFileName[lim];
    for(i = 0; i < lim; i++) {
        clientFileName[i] = '\0';
    }
    strcat(strcat(strcat(clientFileName,"to_client_"),pid),".txt");

    // Open this client file.
    int fd = open(clientFileName,O_RDONLY);

    char buf[SIZE] = {0};
    if((read(fd, buf, SIZE)) == -1) {
        close(fd);
        remove(clientFileName);
        errorHandle();
    }
    if(strcmp(buf, "ERROR0") == 0) {
        printf("CANNOT_DIVIDE_BY_ZERO\n");
    } else if(strcmp(buf, "ERROR1") == 0) {
        printError();
    } else {
        printf("%d\n", atoi(buf));
    }
    if(close(fd) == -1) {
        errorHandle();
    }
    remove(clientFileName);
    exit(0);
}


void alarm_hand(int sig) {
    // timeout -> don't get more signal.
    signal(SIGUSR1, default_hand);

    int i;
    char pid[6] = {0};
    sprintf(pid, "%d", getpid());
    int lim = (int)strlen(pid) + 15;
    char clientFileName[lim];
    for(i = 0; i < lim; i++) {
        clientFileName[i] = '\0';
    }
    strcat(strcat(strcat(clientFileName,"to_client_"),pid),".txt");
    remove(clientFileName);

    printf("%s", TIMEOUT_MSG);
    exit(-1);
}


int main(int argc, char* argv[]) {

    if(argc != 5) {
        errorHandle();
    }

    int fdOut;
    int i = 0;
    // Open srvFile with O_CREAT + O_EXCL -> only one client can open.
    if ((fdOut = open(SRV_FILE,O_CREAT | O_EXCL | O_RDWR | O_APPEND, 0644)) == -1) {
        while (i < 10) {
            sleep(generateNum());
            if ((fdOut = open(SRV_FILE,O_CREAT | O_EXCL | O_RDWR | O_APPEND, 0644)) != -1) {
                break;
            }
            i++;
        }
    }
    if (i == 10) {
        errorHandle();
    }

    writeParams(fdOut, argv);
    pid_t srvPid = atoi(argv[1]);

    signal(SIGUSR1, signal_hand);
    signal(SIGALRM, alarm_hand);

    kill(srvPid , SIGUSR1);
    alarm(TIMEOUT_SEC);

    pause();

    return 0;
}