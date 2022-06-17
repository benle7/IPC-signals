//Ben Levi 318811304

#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <sys/fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>


#define SIZE 30

char* ERROR_MSG =  "ERROR_FROM_EX4\n";
char* SRV_FILE = "to_srv.txt";
int TIMEOUT_SEC = 60;
char* TIMEOUT_MSG = "The server was closed because no service request was received for the last 60 seconds\n";


void printError() {
    printf("%s", ERROR_MSG);
}
void errorHandle() {
    printError();
    exit(-1);
}
void default_hand(int sig) {
    signal(SIGUSR1, default_hand);
}

void signal_hand(int sig) {
    // Stop the last alarm timer.
    alarm(0);
    signal(SIGUSR1, signal_hand);

    pid_t pid = fork();
    if (pid < 0) {
        // Fork failed.
        printError();

    } else if (pid == 0) {
        int fd;
        // The server child process open the srvFile and read the arguments.
        if ((fd = open(SRV_FILE, O_RDONLY)) == -1) {
            errorHandle();
        }
        char buf[SIZE] = {0};
        if((read(fd, buf, SIZE)) == -1) {
            close(fd);
            remove(SRV_FILE);
            errorHandle();
        }
        close(fd);
        // remove the file for enable access to the next clients.
        remove(SRV_FILE);

        char* arguments[4];
        char* token = strtok(buf, "\n");
        int i = 0;
        while (i < 4) {
            arguments[i] = token;
            i++;
            token = strtok(NULL,"\n");
        }

        pid_t clientPid = atoi(arguments[0]);
        int num1 = atoi(arguments[1]);
        int operator = atoi(arguments[2]);
        int num2 = atoi(arguments[3]);
        int flag = 1;

        int result = 0;
        if(operator == 1) {
            result = num1 + num2;
        } else if(operator == 2) {
            result = num1 - num2;
        } else if(operator == 3) {
            result = num1 * num2;
        } else if(operator == 4) {
            if (num2 == 0) {
                // can't calculate it.
                flag = 0;
            } else {
                result = num1 / num2;
            }
        } else {
            flag = 2;
        }

        char resultStr[SIZE] = {0};
        sprintf(resultStr, "%d", result);

        // 15 -> "to_client_" + ".txt".
        int lim = (int)strlen(arguments[0]) + 15;
        char clientFileName[lim];
        for(i = 0; i < lim; i++) {
            clientFileName[i] = '\0';
        }
        strcat(strcat(strcat(clientFileName,"to_client_"),arguments[0]),".txt");
        int fdOut;
        if((fdOut = open(clientFileName,
                         O_CREAT | O_RDWR | O_APPEND | O_TRUNC,
                         0644)) == -1) {
            errorHandle();
        }
        if(flag == 0) {
            if(write(fdOut, "ERROR0", 6) == -1) {
                close(fdOut);
                errorHandle();
            }
        } else if(flag == 2) {
            if(write(fdOut, "ERROR1", 6) == -1) {
                close(fdOut);
                errorHandle();
            }
        } else {
            if(write(fdOut, resultStr, strlen(resultStr)) == -1) {
                close(fdOut);
                errorHandle();
            }
        }
        if(close(fdOut) == -1) {
            printError();
        }
        // Send signal to the client.
        kill(clientPid , SIGUSR1);
        exit(0);

    } else {
        // Parent Process Not Wait.
        // Take all Zombies before exit (alarm timeout).
    }
}


void alarm_hand(int sig) {
    signal(SIGUSR1, default_hand);
    // wait for child processes (Zombies)
    //waitpid(0, NULL, 0);
    while (wait(NULL) != -1) {}

    remove(SRV_FILE);
    printf("%s", TIMEOUT_MSG);
    exit(0);
}


int main() {
    remove(SRV_FILE);
    signal(SIGUSR1, signal_hand);
    signal(SIGALRM, alarm_hand);

    while (true) {
        alarm(TIMEOUT_SEC);
        pause();
    }
}
