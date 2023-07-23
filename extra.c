#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/wait.h>

static int signalGiven = 0;

// Exit key detected 
void quitHanlder() {
	printf("Child is quitting.\n");
	exit(0);
}

// Stop printing when child gets interrupt sig
void parentHanlder(int sig) {
	signalGiven = 0;
}

// Child gets signal from parent
// Print and don't ignore interrupt anymore
void childHanlder(int sig) {
	signalGiven = 1;
	signal(SIGINT, parentHanlder);
}

//Get new message from stdin
void sigDetected(int sig) {
	printf("Interrupt received, enter new message: ");
	signalGiven = 1;
	fflush(stdout); //Need fflush to output
}

//Used for alarm
void handler(int sig) {}

// Delay x amount of time between each message
void customDelay(int lag) {
	signal(SIGALRM, handler);
	alarm(lag);
	pause();
}

int main(int argc, char const *argv[]) {
	int status;
	int fd[2];
	int pid;

	//Create a pipe
	if(pipe(fd) != 0) {
		printf("%s\n", strerror(errno));
		exit(0);
	}

	//Create child process
	if((pid = fork()) == 0) {
		//Ignore SIGINT (ctrl + c)
		if(signal(SIGINT, SIG_IGN) == SIG_ERR) {
			printf("%s\n", strerror(errno));
			exit(0);
		} 
		char buffer[512];
		char message[512];

		//Arrays initialized with 0s
		memset(buffer, 0, 512);
		memset(message, 0, 512);

		//Detected exit key (ctrl + \)
		if(signal(SIGQUIT, quitHanlder) == SIG_ERR) {
			printf("%s\n", strerror(errno));
			exit(0);
		}
		//Sig received from parent
		if(signal(SIGFPE, childHanlder) == SIG_ERR) {
			printf("%s\n", strerror(errno));
			exit(0);
		}
		//Interupt sig received
		if(signal(SIGINT, parentHanlder) == SIG_ERR) {
			printf("%s\n", strerror(errno));
			exit(0);
		}

		int lag = 5; //Default delay time
		for(;;) {
			close(fd[1]);
			int readThis = read(fd[0], buffer, 512);
			if(readThis == -1) {
				printf("%s\n", strerror(errno));
				exit(0);
			}
			lag = 5;
            int pos = 0;
            char num[512];
            char actualLag[512];

            memset(num, 0, 512);
            memset(actualLag, 0, 512*sizeof(int));

            //Check if first token is an integer
            if(isdigit(buffer[pos])) {
                lag = buffer[pos] - '0';
                sprintf(num, "%d", lag);
                strcat(actualLag, num);
                pos++;

                //check if delay is more than one digit
                while(1) {
                    if(isdigit(buffer[pos])) {
                        lag = buffer[pos] - '0';
                        sprintf(num, "%d", lag);
                        strcat(actualLag, num);
                        pos++;
                    }
                    else { break; }
                }
                lag = atoi(actualLag);

				int n = 0;
				for(int i = pos+1; i < 512; i++) {
					message[n] = buffer[i];
					n++;
				}
				strcpy(buffer, message);
			}
			//Else get whole message
			else {
				lag = 5;
				int n = 0;
				for(int i = 0; i < 512; i++) {
					message[n] = buffer[i];
					n++;
				}
				strcpy(buffer, message);
			}
			//Start printing (signal received)
			while(signalGiven == 1) {
				printf("%s", buffer);
				customDelay(lag);
			}
			memset(buffer, 0, 512);
			memset(message, 0, 512);
		}
		close(fd[0]);
	}
	else if (pid == -1) {
		printf("%s\n", strerror(errno));
		exit(0);
	}

	//Parent gets input and writes to child
	else {
		char inpMessage[512];
		char exitInput[] = "exit";

		for(;;) {
			//Sig detected, get new message
			if(signal(SIGINT, sigDetected) == SIG_ERR) {
				printf("%s\n", strerror(errno));
				exit(0);
			}
			//Close off parent
			close(fd[0]);

			//Store message into inpMessage
			if(fgets(inpMessage, 512, stdin) == NULL) {
				printf("%s\n", strerror(errno));
				exit(0);
			}
			//"exit" detected as input, signal child to quit
			if(strncmp(exitInput, inpMessage, 4) == 0) {
				kill(pid, SIGQUIT);
				break; //Break out and wait 
			}
			//Write to child
			write(fd[1], inpMessage, strlen(inpMessage)+1);
			signalGiven = 1;

			//Signal child to write
			kill(pid, SIGFPE);
		}
		close(fd[1]);
		wait(&status);
		exit(0);
	}
	return 0;
}
