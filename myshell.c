/****************************************************************
 * Name        : Kenneth Surban                                 *
 * Class       : CSC 415                                        *
 * Date        : 10/20/18                                       *
 * Description :  Writting a simple bash shell program          *
 *                that will execute simple commands. The main   *
 *                goal of the assignment is working with        *
 *                fork, pipes and exec system calls.            *
 ****************************************************************/

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h> // include type "bool"
#include <sys/wait.h> // include function "waitpid()"

#define BUFFERSIZE 256
#define PROMPT "myShell >> "
#define PROMPTSIZE sizeof(PROMPT)

int main(int* argc, char** argv)
{
	int bytesRead = 0;
	do 
	{
	char *input = (char *)malloc(BUFFERSIZE * sizeof(char));
	char *buf_copy  = (char *)malloc(BUFFERSIZE * sizeof(char));
	bool leftArrowOperatorValid = false;
	bool pipeOperatorValid = false;
	int countRightArrowOperator = 0;

	int myargc;
	//char** myargv = (char *)malloc(BUFFERSIZE * sizeof(char));
	char ** myargv = calloc(200, BUFFERSIZE);
	int index = 0;
	char *token;
	char** arrTokens;

	char** pipeTokens; 
	int pipeTokensSize = 0;
	//char** leftPipe = (char *)malloc(BUFFERSIZE * sizeof(char));
	char ** leftPipe = calloc(200,BUFFERSIZE);
	int leftPipeSize = 0;
	//char** rightPipe = (char *)malloc(BUFFERSIZE * sizeof(char));
	char ** rightPipe = calloc(200,BUFFERSIZE);
	int rightPipeSize = 0;

	//Note: i guess i need to figure how to dynamically allocate
	//a 2-d array... 

	char *output;

	int inputFile = 0;
	int outputFile = 0;
	int file_descriptor = 0;
	char *file_name;

	pid_t pidOne;// = fork();
	pid_t pidTwo;// = fork(); 
	int fd[2];
	char pwd[1024];
	bool backgroundProcess = false;

	printf("%s", PROMPT);
	fflush(stdout); // Reset buffer since PROMPT doesn't end in \n

	//char *input = (char *)malloc(BUFFERSIZE * sizeof(char));

	//getline(&input, &BUFFERSIZE, stdin);
	//printf("'%s'\n", input);

	bytesRead = read(STDIN_FILENO, input, BUFFERSIZE);

	if (input < 0) {
		perror("Error reading input\n");
		return -1;
	}

	//Use strcmp(string s1, string s2) to compare values of input
	// strcmp returns an integer and if 0 the arguments match
	if(strcmp(input, "exit\n") == 0 ) {
		return -1;
	}
	else if(strcmp(input, "\n") == 0 ) {
		continue;
	}
	else if(strcmp(input, " \n") == 0) {
		continue;
	}
	else if(strcmp(input, "pwd\n") == 0) {
		if(getcwd(pwd, sizeof(pwd)) != NULL) {
			printf("%s\n", pwd );
			continue;
		}
		else {
			perror("No such directory exists");
			continue;
		}
	}

	// Special Character Definitions:
	// ">": file output descriptor redirection operator
	// "<": file input descriptor redirection operator
	// ">>": file output descriptor append operator
	// "|": pipe operator

	// check input for any special character cases
	for ( int i = 0; i < bytesRead; i++ ) {
		if (input[i] == '<') {
			input[i] = ' ';
			leftArrowOperatorValid = true;
		}

		else if (input[i] == '>') {
			input[i] = ' ';
			countRightArrowOperator++;
		}

		else if (input[i] == '|') {
			pipeOperatorValid = true;
		}
	}

	/*
	char **myargv = malloc(BUFFERSIZE * sizeof(char));
	int myargc = 0;
	int index = 0;
	char *token;
	*/

	if(pipeOperatorValid) {
		memcpy(buf_copy, input, bytesRead);
	}

	//seperate input via white space + \n
	/*token = strtok(input, " \n");

	while( token != NULL ) {
		arrTokens[index] = token;
		token = strtok(NULL, " \n");
		index++;
		myargc++;
		printf("%s\n", token);
	}*/

	for (char *token = strtok(input, " \n"); token; token = strtok(NULL, " \n")){
		*arrTokens++ = token;
		//index++;
		//myargc++;
		printf("%s\n", token);
	}

	if(pipeOperatorValid) {

		arrTokens = pipeTokens;
		for (char *token = strtok(buf_copy, "|"); token; token = strtok(NULL, " \n")){
			*arrTokens++ = token;
			//index++;
			myargc++;
		}

		arrTokens = leftPipe;
		for (char *token = strtok(pipeTokens[0], "\n"); token; token = strtok(NULL, "|\n")){
			*arrTokens++ = token;
			//index++;
			myargc++;
		}

		arrTokens = rightPipe;
		for (char *token = strtok(pipeTokens[1], " \n"); token; token = strtok(NULL, " \n")){
			*arrTokens++ = token;
			//index++;
			myargc++;
		}
	}

	/////////////
	//Check and handle any special characters detected
	// "<": file input descriptor redirection operator
	if(leftArrowOperatorValid) {
		output = myargv[myargc - 1];
		inputFile = open(output, O_RDONLY);
		if(inputFile < 0) {
			perror("Error creating input file\n");
			return -1;
		}
		else {
			myargv[myargc -1] = NULL;
			myargc--;
		}
	}

	// ">": file output descriptor redirection operator
	if (countRightArrowOperator == 1) {
		output = myargv[myargc - 1];
		outputFile = open(output, O_WRONLY | O_CREAT | O_TRUNC);
		if(outputFile < 0) {
			perror("Error creating output file\n");
			return -1;
		}
		else {
			myargv[myargc -1] = NULL;
			myargc--;
		}
	}

	// ">>": file output descriptor append operator	
	if (countRightArrowOperator == 2) {
		output = myargv[myargc - 1];
		outputFile = open(output, O_WRONLY | O_CREAT | O_APPEND);
		if(outputFile < 0) {
			perror("Error creating output file\n");
			return -1;
		}
		else {
			myargv[myargc -1] = NULL;
			myargc--;
		}
	}

	// 'cd': change directory
	// if (argc > 0 && (strcmp(myargv[0], "cd") == 0 )) {
	// 	char* cd = myargv[1];

	// 	if(chdir(cd) < 0) {
	// 		perror("No such directory exists...");
	// 	}
	// 	else if(argc > 0 && strcmp(myargv[0], "pwd") == 0) {
	// 		printf("we did it");
	// 		if(getcwd(pwd, sizeof(pwd)) == NULL) {
	// 			perror("No such directory exists...");
	// 		}
	// 		else {
	// 			printf("In directory:%s\n", pwd);
	// 		}
	// 	}
	// 	continue;
	// }

	// // '&': run command in background process 
	// if ( strcmp(myargv[myargc - 1], "&") == 0 ) {
	// 	backgroundProcess = true;
	// 	myargv[myargc - 1] = NULL;
	// 	myargc--;
	// }

	if((pidOne = fork()) < 0) {
		perror("Fork failed");
		return -1;
	}

	if (pidOne == 0) {

		if (pipeOperatorValid) {

			if(pipe(fd) < 0) {
				perror("Cannot create pipe");
				return -1;
			}

			if ((pidTwo = fork()) < 0) {
				perror("Fork failed");
				return -1;
			}

			if (pidTwo == 0) {
				int copy_fd = dup2(fd[0], STDIN_FILENO);
				if (copy_fd < 0) {
					perror("Error in creating child\n");
					return -1;
				}
				close(fd[1]);

				//Return error on command+arguments to execute
				//Use execvp, which executes a command+arg like
				//so...   execvp( cmd, arg );
				int execute = execvp(rightPipe[0], rightPipe);

				if (execute < 0 ) {
					printf("Error executing child cmd\n");
					return -1;
				}

				return 0;
			}

		}

		if (pipeOperatorValid) {
			dup2(fd[1], STDOUT_FILENO);
			close(fd[0]);

			file_descriptor = open(file_name, O_RDONLY);
			dup2(file_descriptor, STDIN_FILENO);
			if (execvp(leftPipe[0], leftPipe) < 0) {
				printf("Error executing child cmd\n");
				return -1;
			}

			return 0;
		
		}
		else {

			if (leftArrowOperatorValid) {
				if (dup2(outputFile, STDOUT_FILENO) < 0 ) {
					perror("Failure on generating output\n");
				}
			}

			if ((countRightArrowOperator == 1) || (countRightArrowOperator == 2)) {
				if (dup2(outputFile, STDOUT_FILENO) < 0) {
					perror("Failure on generating output\n");
				}
			}

			// "pwd": print-working-directory
			if(strcmp(myargv[0], "pwd") == 0) {
				if(getcwd(pwd, sizeof(pwd)) != NULL) {
					printf("%s\n", pwd );
				}
				else {
					perror("Cannot print current working directory");
				}
			} 
			else if(execvp(myargv[0], myargv) < 0) {
				perror("No such command found");
			}
			return 0;
		}
	}
	else {
		if (backgroundProcess == false) {
			waitpid(pidOne,NULL,0);
		}
	}
	// reset and free memory
	free(input);
	free(myargv);
	free(leftPipe);
	free(rightPipe);

	} while(bytesRead > 0);

	return 0;
}