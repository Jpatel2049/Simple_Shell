#ifndef MYSHELL_H_
#define MYSHELL_H_

/* Command types. */
#define PIPE "|"
#define BACK "&"
#define LEFT "<"
#define RIGHT_1 ">"
#define RIGHT_2 ">>"

/* Function declarations. */
void start_shell();
void read_batchfile(char *);
char** split_line(char *);
int execute_command(char**);
char* trim_space(char *);
void external_command(char**, int);
void redir(char**);
void handle_pipe(char**);
void background(char**);

/* Internal commands. */
void change_dir(char **);
int quit_prog();
void clear_screen();
void dir_list(char*);
void echo(char**);
void pause_shell();
void set_path(char**);
void print_environ();
void print_help();

#endif
