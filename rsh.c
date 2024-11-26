#include <stdio.h>
#include <stdlib.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>

#define N 13

extern char **environ;
char uName[20];

char *allowed[N] = {"cp", "touch", "mkdir", "ls", "pwd", "cat", "grep", "chmod", "diff", "cd", "exit", "help", "sendmsg"};

struct message
{
	char source[50];
	char target[50];
	char msg[200];
};

void terminate(int sig)
{
	printf("Exiting....\n");
	fflush(stdout);
	exit(0);
}

void sendmsg(char *user, char *target, char *msg)
{
	// TODO:
	// Send a request to the server to send the message (msg) to the target user (target)
	// by creating the message structure and writing it to server's FIFO
	struct message reqestServer;
	strcpy(reqestServer.source, user);
	strcpy(reqestServer.target, target);
	strcpy(reqestServer.msg, msg);
	int FIFO = open("serverFIFO", O_WRONLY);
    write(FIFO, &reqestServer, sizeof(reqestServer));
	close(FIFO);
}

void *messageListener(void *arg)
{
	// TODO:
	// Read user's own FIFO in an infinite loop for incoming messages
	// The logic is similar to a server listening to requests
	// print the incoming message to the standard output in the
	// following format
	// Incoming message from [source]: [message]
	// put an end of line at the end of the message
	struct message tempMessage;			 // create message
	char FIFO[256];						 // store FIFO name
	sprintf(FIFO, "%s", uName);			 // set FIFO to username
	int openFIFO = open(FIFO, O_RDONLY); // Open user's FIFO for reading

	while (1)
	{
		size_t messageSize = sizeof(tempMessage);						// message size
		size_t readMessage = read(openFIFO, &tempMessage, messageSize); // read message

		if (readMessage != 0) // print message if one is sent
		{
			printf("Incoming message from %s: %s\n", tempMessage.source, tempMessage.msg);
		}
	}
	close(openFIFO);
	pthread_exit((void *)0);
}

int isAllowed(const char *cmd)
{
	int i;
	for (i = 0; i < N; i++)
	{
		if (strcmp(cmd, allowed[i]) == 0)
		{
			return 1;
		}
	}
	return 0;
}

int main(int argc, char **argv)
{
	pid_t pid;
	char **cargv;
	char *path;
	char line[256];
	int status;
	posix_spawnattr_t attr;

	if (argc != 2)
	{
		printf("Usage: ./rsh <username>\n");
		exit(1);
	}
	signal(SIGINT, terminate);

	strcpy(uName, argv[1]);

	// TODO:
	// create the message listener thread
	pthread_t listener;
	pthread_create(&listener, NULL, messageListener, NULL); // create the listener

	while (1)
	{

		fprintf(stderr, "rsh>");

		if (fgets(line, 256, stdin) == NULL)
			continue;

		if (strcmp(line, "\n") == 0)
			continue;

		line[strlen(line) - 1] = '\0';

		char cmd[256];
		char line2[256];
		strcpy(line2, line);
		strcpy(cmd, strtok(line, " "));

		if (!isAllowed(cmd))
		{
			printf("NOT ALLOWED!\n");
			continue;
		}

		if (strcmp(cmd, "sendmsg") == 0)
		{
			// TODO: Create the target user and
			// the message string and call the sendmsg function

			// NOTE: The message itself can contain spaces
			// If the user types: "sendmsg user1 hello there"
			// target should be "user1"
			// and the message should be "hello there"

			// if no argument is specified, you should print the following
			// printf("sendmsg: you have to specify target user\n");
			// if no message is specified, you should print the followingA
			// printf("sendmsg: you have to enter a message\n");
			char *targetValue = strtok(NULL, " ");	// store target
			char *messageValue = strtok(NULL, ""); // store message can't put space

			if (!targetValue || !messageValue)
			{ // if missing one of the values
				if (!targetValue)
				{
					printf("sendmsg: you have to specify target user\n"); // if no argument
				}
				else
				{
					printf("sendmsg: you have to enter a message\n"); // if no message
				}
				continue; // continue to next loop
			}
			sendmsg(uName, targetValue, messageValue);
			continue;
		}

		if (strcmp(cmd, "exit") == 0)
			break;

		if (strcmp(cmd, "cd") == 0)
		{
			char *targetDir = strtok(NULL, " ");
			if (strtok(NULL, " ") != NULL)
			{
				printf("-rsh: cd: too many arguments\n");
			}
			else
			{
				chdir(targetDir);
			}
			continue;
		}

		if (strcmp(cmd, "help") == 0)
		{
			printf("The allowed commands are:\n");
			for (int i = 0; i < N; i++)
			{
				printf("%d: %s\n", i + 1, allowed[i]);
			}
			continue;
		}

		cargv = (char **)malloc(sizeof(char *));
		cargv[0] = (char *)malloc(strlen(cmd) + 1);
		path = (char *)malloc(9 + strlen(cmd) + 1);
		strcpy(path, cmd);
		strcpy(cargv[0], cmd);

		char *attrToken = strtok(line2, " "); /* skip cargv[0] which is completed already */
		attrToken = strtok(NULL, " ");
		int n = 1;
		while (attrToken != NULL)
		{
			n++;
			cargv = (char **)realloc(cargv, sizeof(char *) * n);
			cargv[n - 1] = (char *)malloc(strlen(attrToken) + 1);
			strcpy(cargv[n - 1], attrToken);
			attrToken = strtok(NULL, " ");
		}
		cargv = (char **)realloc(cargv, sizeof(char *) * (n + 1));
		cargv[n] = NULL;

		// Initialize spawn attributes
		posix_spawnattr_init(&attr);

		// Spawn a new process
		if (posix_spawnp(&pid, path, NULL, &attr, cargv, environ) != 0)
		{
			perror("spawn failed");
			exit(EXIT_FAILURE);
		}

		// Wait for the spawned process to terminate
		if (waitpid(pid, &status, 0) == -1)
		{
			perror("waitpid failed");
			exit(EXIT_FAILURE);
		}

		// Destroy spawn attributes
		posix_spawnattr_destroy(&attr);
	}
	return 0;
}
