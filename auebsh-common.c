/* Konstantinos Lamprakis 12/01/2018 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "auebsh.h"

int stdout_descriptor, stdin_descriptor; // descriptors for stdin, stdout.
int size; // size of char ** array which contains command and its arguments.

// read command from terminal, check to be less than 255 characters and return this line.
char * read_line(const char* shell_prommt){

  char *line = NULL; // hold the command.
  ssize_t bufsize = 0; // help temporary for reading the command.

    // check if first character is eof, so shell should exit.
    char eof_check = getc(stdin); // take first character.
    if((eof_check == EOF) && (feof(stdin))){ // if its eof exit.
        printf("\n\n---- EOF reached. Exit from shell. ----- \n\n");
        exit(0);
    }else{
        ungetc(eof_check, stdin); // else put back the character.
    };

  // read command and make input control for maximum number of characters equal to 255.
  while(getline(&line, &bufsize, stdin)>255){
        printf("Error: too many characters. Maximum number of valid characters is 255.\nPlease try again.%s", shell_prommt);
  }
  return line;
}

// take as argument a command,split it to commands name and parameters and return an array with its parts.
char ** format_line(char *line){

	int tempSize = 10; // tempSize is a temporary buffer to hold maximum number of parameters. It can increased if required.
	size = 0; // position is the actual number of parameters.
	char **args = malloc(tempSize * sizeof(char*)); // args allocated memory for tempSize parameters.
	char *token; // token is each parameter, parameters are seperated wiith " " (or "\n" when user hit enter).

  	if (!args) { // check if memory isn't enough for allocation.
    	perror("Error: An error occured during memory allocation.\n");
    	exit(1);
	}

  	token = strtok(line, " \n"); // take first parameter.

 	while (token != NULL) { // take the rest parameters.
    	args[size] = token; // store parameteres to args array.
    	size++; // increase index of args array.

		if (size >= tempSize) { // if the actual parameters are more than allocated space, then alocate space for 5 more arguments.
			tempSize += 5;
			args = realloc(args, tempSize * sizeof(char*));
		  	if (!args) { // check if memory isn't enough for allocation.
		    	perror("Error: An error occured during memory allocation.\n");
		    	exit(1);
		  	}
		}

		token = strtok(NULL, " \n"); // take next parameter.
	}

	args[size] = NULL; // set NULL value as the last element of args array.

  	return args;

}

// take as argument an array with command name and its parameters and executes it.
int execute(char **args){

	pid_t pid; // pid is the id of any process, 0 for child, negative for error and positive for parent.
    pid_t waitPid;
	int status;

	pid = fork(); // create new child process, same as parent.
	if (pid == 0) { // Child process.
		execvp(args[0], args); // Normally this commands shouldn't returned.
        perror("Error: An error occured during command's execution.\n");
		exit(1);

	} else if (pid < 0) { // An error occured.
		perror("Error: An error occured during creation of new process for command's execution.\n");

	}else { // Parent process.
		waitPid = wait(&status); // wait termination of child and release its resource, so doesn't left zombie process.
		if (waitPid == -1) {
			perror("Error: An error occured while waiting for children process termination.\n");
			return -1;
		}
	}
}

// take as argument an array with command name and its parameters, makes proper redirections and return pure command as array.
char ** make_redirections(char **args){

    int j = 0;
    char **result;

    if(size < 10){
        result = malloc(10*sizeof(char*));
    }else{
        result = malloc(size*sizeof(char*));
    }


    if (!result) { // check if memory isn't enough for allocation.
    	perror("Error: An error occured during memory allocation.\n");
    	exit(1);
	}

    for(int i = 0; args[i] != NULL; i++){

        if(strcmp(args[i],">") == 0){ // stdout redirection

            /*
            O_WRONLY: open file for write only puprose.
            O_CREAT: create this file, if not exists.
            0666: read-write rights.
            */

            int file_descriptor= open(args[++i], O_WRONLY|O_CREAT, 0666); // open file for redirection.
            if(file_descriptor < 0) perror("Error: An error occured during opening file for redirection.\n"); // chack for error.

            if(dup2(file_descriptor, 1) < 0) perror("Error: An error occured during redrection.\n"); // redirect stdout(which has as number descriptor 1) to file with file_descriptor.

        }else if(strcmp(args[i],"<") == 0){

            int file_descriptor= open(args[++i], O_RDONLY); //  O_RDONLY: open file for read only puprose.
            if(file_descriptor < 0) perror("Error: An error occured during opening file for redirection.\n"); // chack for error.

            if(dup2(file_descriptor, 0) < 0) perror("Error: An error occured during redirection.\n"); // redirect stdin(which has as number descriptor 0) to file with file_descriptor.

        }else{
            result[j++] = args[i];
        }
    }

    result[j] = NULL;
    free(args);

    return result;
}

void store_stdin_stdout(void){
    stdin_descriptor = dup(0);
    stdout_descriptor = dup(1);
}

void restore_stdin_stdout(void){
    dup2(stdin_descriptor, 0);
    dup2(stdout_descriptor, 1);
}

void close_stdin_stdout(void){
    close(stdin_descriptor);
    close(stdout_descriptor);
}


// take care of pipelining and executes all commands.
void execute_pipelines(char **args){

    // Declarations and preparation.
    store_stdin_stdout(); // make back up of stdin, stdout descriptors.

    char **temp; // stores temporary arguments of each command before pipeline.
    int j=0; // iterator-index of temp array.

    // allocate space for temp.
    if(size < 10){
       temp = malloc(10*sizeof(char*));
    }else{
        temp = malloc(size*sizeof(char*));
    }
    if(!temp) { // check if memory isn't enough for allocation.
    	perror("Error: An error occured during memory allocation.\n");
    	exit(1);
	}

    pid_t pid, waitPid; // used by fork and wait.
    int status;

	int desc[2]; // desc = descriptors for pipelining.
    int old_input = 0; // used to connect output of old command to input of new one in pipelining.


	// Pipelining execution.
    for (int i=0; args[i]!= NULL; i++){

        if (strcmp(args[i],"|") == 0){

            if (pipe(desc)==-1){
                perror("Error: An error occured during opening pipelines.");
                return 1;
            }

            temp[j] = NULL; // complete temp array and go for execution of first command.
            pid = fork(); // create new child process, same as parent.

            if (pid == 0) {

                temp = make_redirections(temp); // makes redirection if needed.
                dup2(old_input,0); // read from previous command from pipeline.
                close(desc[0]); // close descriptor for reading.
                dup2(desc[1],1); // exchange stdout with writting descriptor.
                execute(temp); // execute command before pipeline.
                exit(0); // exit from child process.

            } else if (pid < 0) { // An error occured.
                perror("Error: An error occured during creation of new process for command's execution.\n");

            }else { // Parent process.

                waitPid = wait(&status); // wait termination of child and release its resource, so doesn't left zombie process.
                if (waitPid == -1) {
                    perror("Error: An error occured while waiting for children process termination.\n");
                    exit(1);
                }
                close(desc[1]); // close writting descriptor.
                old_input = desc[0]; // update input of next command.
            }
            j = 0; // restore temp array to store next command.

        }else{ // if not reached pipeline yet, just copy argument from args to temp.
            temp[j++] = args[i];
        }
    }

    if (pipe(desc)==-1){
        perror("Error: An error occured during opening pipelines.");
        return 1;
    }

    temp[j] = NULL; // complete temp array and go for execution of first command.
    pid = fork(); // create new child process, same as parent.

    if (pid == 0) { // Child process, will execute first, so its the first command.

        dup2(old_input,0);
        close(desc[0]);
        dup2(stdout_descriptor,1); // exchange stdout with writting descriptor.
        temp = make_redirections(temp); // makes redirection if needed.
        execute(temp); // execte command befre pipeline.
        exit(0); // exit from child process.

    } else if (pid < 0) { // An error occured.
        perror("Error: An error occured during creation of new process for command's execution.\n");

    }else { // Parent process.

        waitPid = wait(&status); // wait termination of child and release its resource, so doesn't left zombie process.
        if (waitPid == -1) {
            perror("Error: An error occured while waiting for children process termination.\n");
            exit(1);
        }
        close(desc[1]);
        old_input = desc[0];
    }

    free(temp);
    restore_stdin_stdout();
}
