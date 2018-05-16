/* Konstantinos Lamprakis 12/01/2018 */

#include <stdio.h>
#include <stdlib.h>
#include "auebsh.h"

// methods declaration.
void run_shell(void);

int main(){
    printf("This program is a shell named \"auebsh5\" for linux.\n");
    run_shell();

    return 0;
}

// start shell for first time.
void run_shell(void){
    char *line; // holds the command from terminal.
    char **args; // holds command name and its parameters.
    const char * shell_prommt = "\nauebsh5> ";

	while(1){
		printf(shell_prommt);
		line = read_line(shell_prommt); // read command.
		args = format_line(line); // turn command to proper format for execution.
		execute_pipelines(args); // execute command.
		free(line); // deallocates the memory of line.
		free(args); // deallocates the memory of args.
	}

}

