/* Konstantinos Lamprakis 12/01/2018 */

#ifndef AUEBSH_H_INCLUDED
#define AUEBSH_H_INCLUDED

// declaration of common.c functions used from all shells.
char * read_line(const char* shell_prommt);
char ** format_line(char *line);
int execute(char **args);

char ** make_redirections(char **args);
void store_stdin_stdout(void);
void restore_stdin_stdout(void);
void close_stdin_stdout(void);

void execute_pipelines(char **args);

#endif // AUEBSH_H_INCLUDED
