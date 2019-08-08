#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "myshell.h"

/* Size for buffer . */
size_t buffer_size = 500;

/* Strings for buffer and path. */
char* buffer, *path;

/* Array of strings for paths to search. */
char** path_list;

/* If background process. */
int back = 0;

/* To exit shell . */
int check_exit = 0;

/* Default error message. */
char error_message[30] = "An error has occurred\n";

int main(int argc, char* argv[]) {
  /* Starts shell. */
  if(argc < 2) {
    start_shell();
    /* Reads from batchfile. */
  } else if(argc == 2) {
    read_batchfile(argv[1]);
    /* If greater than 2 args exit. */
  } else {
    exit(1);
  }
  return 0;
}

/* Shell loop. */
void start_shell() {
  /* Allocates space for buffer, path, and path list. */
  buffer = (char *) malloc(buffer_size * sizeof(char *));
  path = (char *) malloc(buffer_size * sizeof(char *));
  path_list = malloc(buffer_size * sizeof(char *));
  if(buffer == NULL || path == NULL || path_list == NULL) {
    write(STDERR_FILENO, error_message, strlen(error_message));
    exit(1);
  }
  int i;
  for(i = 0; i < buffer_size; i++) {
    path_list[i] = (char *) malloc(buffer_size * sizeof(char *));
    if(path_list[i] == NULL) {
      write(STDERR_FILENO, error_message, strlen(error_message));
      exit(1);
    }
  }

  /* Adds /bin to path list by default. */
  strcat(path_list[0], "/bin");

  while(check_exit == 0) {
    /* Gets path and prints shell prompt. */
    path = getcwd(path, buffer_size);
    printf("%s/myshell> ", path);

    /* Gets user input. */
    getline(&buffer, &buffer_size, stdin);

    /* Splits user input and handles command. */
    char** split = split_line(buffer);
    check_exit = execute_command(split);

    /* Free memory allocated in split_line. */
    for(i = 0; i < buffer_size; i++) {
      free(split[i]);
    }
    free(split);
  }

  /* Frees memory allocated for buffer, path, and path list. */
  free(buffer);
  free(path);
  for(i = 0; i < buffer_size;i++) {
    free(path_list[i]);

  }
  free(path_list);

  /* Quits program. */
  exit(0);
}

/* Reads from batchfile. */
void read_batchfile(char* filename) {
  /* Opens batchfile. */
  FILE *fp = fopen(filename, "r");
  if(fp == NULL) {
    write(STDERR_FILENO, error_message, strlen(error_message));
    exit(1);
  }

  /* Allocates space for buffer, path, and path list. */
  buffer = (char *) malloc(buffer_size * sizeof(char *));
  path_list = malloc(buffer_size * sizeof(char *));
  if(buffer == NULL || path_list == NULL) {
    write(STDERR_FILENO, error_message, strlen(error_message));
    exit(1);
  }
  int i;
  for(i = 0; i < buffer_size; i++) {
    path_list[i] = malloc(buffer_size * sizeof(char *));
    if(path_list[i] == NULL) {
      write(STDERR_FILENO, error_message, strlen(error_message));
      exit(1);
    }
  }

  /* Adds /bin to path list by default. */
  strcat(path_list[0], "/bin");

  /* Reads from file until EOF or quit. */
  while(getline(&buffer, &buffer_size, fp) != EOF) {
    /* Splits input and handles command. */
    char** split = split_line(buffer);
    check_exit = execute_command(split);
    if(check_exit != 0) {
      break;
    }
  }

  /* Frees memory allocated for buffer, path, and path list. */
  for(i = 0; i < buffer_size; i++) {
    free(path_list[i]);
  }
  free(path_list);
  free(buffer);
  fclose(fp);

  /* Exits program. */
  exit(0);
}

/* Splits line into tokens. */
char** split_line(char* line) {
  char* temp;
  int i = 0, j;
  /* Allocates space for output. */
  char** output = malloc(sizeof(char*) * buffer_size);
  if(output == NULL) {
    write(STDERR_FILENO, error_message, strlen(error_message));
    exit(1);
  }
  for(j = 0; j < buffer_size; j++) {
    output[j] = (char *) malloc(buffer_size * sizeof(char *));
    if(output == NULL) {
      write(STDERR_FILENO, error_message, strlen(error_message));
      exit(1);
    }
  }

  /* Splits input by spaces and tabs. */
  temp = strtok(line, "  \t");
  while (temp != NULL) {
      /* Removes any leading or trailing white space. */
      temp = trim_space(temp);
      /* Copies token to output. */
      strcpy(output[i++], temp);
      /* Moves to next token. */
      temp = strtok(NULL, "  \t");
  }

  output[i] = NULL;
  return output;
}

/* Handles the command and then executes them. */
int execute_command(char** input) {
  /* Check for symbols */
  int check_pipe = 0, check_left = 0, check_right = 0, check_right2 = 0,
    check_back = 0, i;

    for(i = 0; input[i] != NULL; i++) {
      if(strcmp(input[i], PIPE) == 0) {
        check_pipe++;
      } else if(strcmp(input[i], LEFT) == 0) {
        check_left++;
      } else if(strcmp(input[i], RIGHT_1) == 0) {
        check_right++;
      } else if(strcmp(input[i], BACK) == 0) {
        check_back++;
      } else if(strcmp(input[i], RIGHT_2) == 0) {
        check_right2++;
      }
    }

    /* Command only contains I/0 redirection. */
    if((check_left != 0 || check_right != 0 || check_right2 != 0) && check_pipe == 0 && check_back == 0) {
      redir(input);
      /* Command only contains piping and/or I/O redirection. */
    } else if(check_pipe != 0 && check_back == 0) {
        handle_pipe(input);
      /* Command is a background process. */
    } else if(check_back != 0) {
        background(input);

   /* Command does not have I/O redirection, piping, and is not a background process. */
    } else {

      /* Checks if built in command, if not it is an external command. */
      if(input[0] == NULL) {
        return 0;
      } else if(strcmp(input[0],"quit") == 0) {
          return quit_prog();
      } else if(strcmp(input[0],"cd") == 0) {
          change_dir(input);
      } else if(strcmp(input[0],"clr") == 0) {
          clear_screen();
      } else if(strcmp(input[0],"dir") == 0) {
          dir_list(input[1]);
      } else if(strcmp(input[0],"echo") == 0) {
          echo(input);
      } else if(strcmp(input[0],"pause") == 0) {
          pause_shell();
      } else if(strcmp(input[0],"path") == 0) {
          set_path(input);
      } else if(strcmp(input[0],"environ") == 0) {
          print_environ();
      } else if(strcmp(input[0], "help") == 0) {
          print_help();
      } else {
          external_command(input, back);
      }
    }
  return 0;
}

/* Trims white space. */
char* trim_space(char * input) {
  char *end;

  // Trim leading space
  while(isspace((unsigned char)*input)) {
    input++;
  }

  if(*input == 0) {
    return input;
  }

  // Trim trailing space
  end = input + strlen(input) - 1;
  while(end > input && isspace((unsigned char)*end)) {
    end--;
  }

  // Write new null terminator character
  end[1] = '\0';
  return input;
}

/* Internal commands. */

/* Change directory. */
void change_dir(char** input) {
  /* If arg empty, list current directory. */
  if(input[1] == NULL) {
    printf("%s\n", path);
  } else {
    if(chdir(input[1])) {
      write(STDERR_FILENO, error_message, strlen(error_message));
    }
  }
}

/* Quits the shell. */
int quit_prog() {
  return 1;
}

/* Clears the screen. */
void clear_screen() {
  printf("\033c" );
}

/* Lists the contents of the directory. */
void dir_list(char* input) {
  DIR *d;
  struct dirent *dir;
  /* Opens directory and lists contents. */
  d = opendir(input);
  if(d) {
    while((dir = readdir(d)) != NULL) {
      printf("%s\n", dir->d_name);
    }
    /* Closes directory. */
    closedir(d);
  }
}

/* Prints the args. */
void echo(char** input) {
  /* Justs prints new line if args is NULL. */
  if(input[1] == NULL) {
    printf("\n");
  } else {
    int i;
    for(i = 1; input[i] != NULL; i++) {
      printf("%s ", input[i]);
    }
    /* Prints new line at end. */
    printf("\n");
  }
}

/* Pauses the shell. */
void pause_shell() {
  printf("Press Enter to continue.\n");
  while (getchar() != '\n');
}

/* Sets path list. */
void set_path(char** input) {
  int i, j;

  /* Clears path list. */
  for(i = 0; i < buffer_size; i++) {
    strcpy(path_list[i],"");
  }
  /* Copies new path list from input. */
  if(input[1] != NULL) {
    for(i = 0, j = 1; input[j] != NULL; i++, j++) {
      strcpy(path_list[i],input[j]);
    }
  }
}

/* Prints the environment strings. */
void print_environ() {
  extern char **environ;
  int i = 0;
  while(environ[i]) {
    printf("%s\n", environ[i++]);
  }
}

/* Prints the help text. */
void print_help() {
  FILE *fp;
  char c;
  /* Opens and prints contents. */
  fp = fopen("readme", "r");
  if(fp == NULL) {
    write(STDERR_FILENO, error_message, strlen(error_message));
    return;
  }
  c = fgetc(fp);
   while (c != EOF) {
     printf ("%c", c);
     c = fgetc(fp);
   }
   /* Prints new line and closes file. */
   printf("\n");
   fclose(fp);
}

/* External commands. */
void external_command(char** input, int b) {
  if(input[0] == NULL) {
    return;
  }
  pid_t pid;
  int i, found = 0;
  /* Search through path list to find command in directory. */
  for(i = 0; i < buffer_size; i++) {
    char * temp = malloc(strlen(input[0]) * strlen(path_list[i]) + 2);
    /* Combine the path name and the command name. */
    strcpy(temp, path_list[i]);
    strcat(temp, "/");
    strcat(temp, input[0]);
    /* If command found, fork and let child execute. */
    if(access(temp, X_OK) != -1) {
      found++;
      pid = fork();

      /* Fork error. */
      if (pid == -1) {
        write(STDERR_FILENO, error_message, strlen(error_message));
        return;
      /* Child. */
    } else if (pid == 0) {
        if (execvp(input[0], input) < 0) {
            write(STDERR_FILENO, error_message, strlen(error_message));
        }
      /* Parent. */
    } else {
        /* Wait if not background process. */
       if(back == 0) {
          waitpid(pid, NULL, 0);
       }
    }
  }
  free(temp);
 }
 /* If command not found. */
 if(found == 0) {
   write(STDERR_FILENO, error_message, strlen(error_message));
 }
}

/* Handles I/O redirection. */
void redir(char** input) {
  int fd; int cmd = 0, i, args = 0, left_pos = 0, right_pos = 0, right2_pos = 0;
  int in_file, out_file, temp, temp2, j = 0;
  /* Args to left of redirection symbols. */
  char** left = malloc(sizeof(char *) * buffer_size);
  if(left == NULL) {
    write(STDERR_FILENO, error_message, strlen(error_message));
    exit(1);
  }

  /* Finds position of redirection symbols. */
  for(i = 0; input[i] != NULL; i++) {
    if(strcmp(input[i], LEFT) == 0) {
      left_pos = i;
      args++;
    } else if(strcmp(input[i], RIGHT_1) == 0) {
        right_pos = i;
        args++;
    } else if(strcmp(input[i], RIGHT_2) == 0) {
        right2_pos = i;
        args++;
    }

    /* Copies args to the left of symbols to buffer. */
    if(args == 0) {
      left[j] = (char *) malloc(sizeof(char *) * strlen(input[i]) + 1);
      if(left[j] == NULL) {
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
      }
      strcpy(left[j], input[i]);
      j++;
    }
  }

  /* Handles the redirection based on the redirection symbols and
  executes the command. */

  /* < and > symbols found. */
  if(left_pos != 0 && right_pos != 0) {
    in_file = left_pos + 1;
    out_file = right_pos + 1;

    if((fd = open(input[in_file], O_RDONLY , 0755)) == -1) {
      write(STDERR_FILENO, error_message, strlen(error_message));
      return;
    }
    temp = dup(STDIN_FILENO);
    dup2(fd, STDIN_FILENO);
    close(fd);

    if((fd = open(input[out_file], O_RDWR | O_CREAT | O_TRUNC, 0755)) == -1) {
      write(STDERR_FILENO, error_message, strlen(error_message));
      return;
    }
    temp2 = dup(STDOUT_FILENO);
    dup2(fd, STDOUT_FILENO);
    close(fd);

    execute_command(left);

    dup2(temp, STDIN_FILENO);
    close(temp);
    dup2(temp2, STDOUT_FILENO);
    close(temp2);

    /* < and >> symbols found. */
  } else if(left_pos != 0 && right2_pos != 0) {
    in_file = left_pos + 1;
    out_file = right2_pos + 1;

    if((fd = open(input[in_file], O_RDONLY , 0755)) == -1) {
      write(STDERR_FILENO, error_message, strlen(error_message));
      return;
    }
    temp = dup(STDIN_FILENO);
    dup2(fd, STDIN_FILENO);
    close(fd);

    if((fd = open(input[out_file], O_RDWR | O_CREAT | O_APPEND, 0755)) == -1) {
      write(STDERR_FILENO, error_message, strlen(error_message));
      return;
    }
    temp2 = dup(STDOUT_FILENO);
    dup2(fd, STDOUT_FILENO);
    close(fd);

    execute_command(left);

    dup2(temp, STDIN_FILENO);
    close(temp);
    dup2(temp2, STDOUT_FILENO);
    close(temp2);

    /* >> symbol found. */
  } else if(right2_pos != 0) {
      out_file = right2_pos + 1;

      if((fd = open(input[out_file], O_RDWR | O_CREAT | O_APPEND, 0755)) == -1) {
        write(STDERR_FILENO, error_message, strlen(error_message));
        return;
      }
      temp = dup(STDOUT_FILENO);
      dup2(fd, STDOUT_FILENO);
      close(fd);

      execute_command(left);

      dup2(temp, STDOUT_FILENO);
      close(temp);

    /* > symbol found. */
  } else if(right_pos != 0) {
    out_file = right_pos + 1;

    if((fd = open(input[out_file], O_RDWR | O_CREAT | O_TRUNC, 0755)) == -1) {
      write(STDERR_FILENO, error_message, strlen(error_message));
      return;
    }
    temp = dup(STDOUT_FILENO);
    dup2(fd, STDOUT_FILENO);
    close(fd);

    execute_command(left);

    dup2(temp, STDOUT_FILENO);
    close(temp);

    /* < symbol found. */
  } else if(left_pos != 0) {
      in_file = left_pos + 1;

      if((fd = open(input[in_file], O_RDONLY , 0755)) == -1) {
        write(STDERR_FILENO, error_message, strlen(error_message));
        return;
      }
      temp = dup(STDIN_FILENO);
      dup2(fd, STDIN_FILENO);
      close(fd);

      execute_command(left);

      dup2(temp, STDIN_FILENO);
      close(temp);
    /* Anything other combo is not allowed. */
  } else {
    return;
  }
}

/* Handles piping. */
void handle_pipe(char** input) {
  int pipefd[2], i, j = 0, pipe_found = 0, redir_left = 0, redir_right = 0;

  /* Creates the pipe. */
  if(pipe(pipefd) < 0) {
    write(STDERR_FILENO, error_message, strlen(error_message));
    return;
  }

  /* Creates buffers for the left and right of the pipe. */
  char ** left = malloc(sizeof(char *) * buffer_size);
  char ** right = malloc(sizeof(char *) * buffer_size);
  if(left == NULL || right == NULL) {
    write(STDERR_FILENO, error_message, strlen(error_message));
    exit(1);
  }

  /* Loops through input and copies the args to left of the pipe and to the right. */
  for(i = 0; input[i] != NULL; i++) {
    if(strcmp(input[i], PIPE) == 0) {
        pipe_found++;
    } else if(pipe_found == 0) {
        /* I/O redirection to the left of the pipe. */
        if(strcmp(input[i], LEFT) == 0 || strcmp(input[i], RIGHT_1) == 0 || strcmp(input[i], RIGHT_2) == 0) {
          redir_left++;
        }
        left[i] = (char *) malloc(sizeof(char *) * strlen(input[i]) + 1);
        if(left[i] == NULL) {
          write(STDERR_FILENO, error_message, strlen(error_message));
          exit(1);
        }
        strcpy(left[i], input[i]);
    } else if(pipe_found != 0) {
      /* I/O redirection to the right of the pipe. */
      if(strcmp(input[i], LEFT) == 0 || strcmp(input[i], RIGHT_1) == 0 || strcmp(input[i], RIGHT_2) == 0) {
        redir_right++;
      }
        right[j] = (char *) malloc(sizeof(char *) * strlen(input[i]) + 1);
        if(right[j] == NULL) {
          write(STDERR_FILENO, error_message, strlen(error_message));
          exit(1);
        }
        strcpy(right[j], input[i]);
        j++;
    }
  }
    /* No redirection. */
    if(redir_left == 0 && redir_right == 0) {
      int temp = dup(STDOUT_FILENO);
      dup2(pipefd[1], STDOUT_FILENO);
      close(pipefd[1]);
      execute_command(left);
      dup2(temp, STDOUT_FILENO);
      close(temp);

      int temp2 = dup(STDIN_FILENO);
      dup2(pipefd[0], STDIN_FILENO);
      close(pipefd[0]);
      execute_command(right);
      dup2(temp2, STDIN_FILENO);
      close(temp2);

      /* Redirection on left only. */
    } else if(redir_left != 0 && redir_right == 0) {
      int temp = dup(STDOUT_FILENO);
      dup2(pipefd[1], STDOUT_FILENO);
      close(pipefd[1]);
      redir(left);
      dup2(temp, STDOUT_FILENO);
      close(temp);

      int temp2 = dup(STDIN_FILENO);
      dup2(pipefd[0], STDIN_FILENO);
      close(pipefd[0]);
      execute_command(right);
      dup2(temp2, STDIN_FILENO);
      close(temp2);

      /* Redirection on right only. */
    } else if(redir_left == 0 && redir_right != 0) {
      int temp = dup(STDOUT_FILENO);
      dup2(pipefd[1], STDOUT_FILENO);
      close(pipefd[1]);
      execute_command(left);
      dup2(temp, STDOUT_FILENO);
      close(temp);

      int temp2 = dup(STDIN_FILENO);
      dup2(pipefd[0], STDIN_FILENO);
      close(pipefd[0]);
      redir(right);
      dup2(temp2, STDIN_FILENO);
      close(temp2);

      /* Redirection on both sides. */
    } else if(redir_left != 0 && redir_right != 0) {
      int temp = dup(STDOUT_FILENO);
      dup2(pipefd[1], STDOUT_FILENO);
      close(pipefd[1]);
      redir(left);
      dup2(temp, STDOUT_FILENO);
      close(temp);

      int temp2 = dup(STDIN_FILENO);
      dup2(pipefd[0], STDIN_FILENO);
      close(pipefd[0]);
      redir(right);
      dup2(temp2, STDIN_FILENO);
      close(temp2);
    }
}

/* Handles background operations. */
void background(char** input) {
  int i, j = 0;

  /* Sets background to true. */
  back = 1;

  /* Allocates space for temp buffer. */
  char** temp = malloc(sizeof(char *) * buffer_size);
  if(temp == NULL) {
    write(STDERR_FILENO, error_message, strlen(error_message));
    exit(1);
  }
  for(i = 0; i < buffer_size; i++) {
    temp[i] = (char *) malloc(sizeof(char *) * 10);
    if(temp[i] == NULL) {
      write(STDERR_FILENO, error_message, strlen(error_message));
      exit(1);
    }
  }

  /* Loops through input. */
  for(i = 0; input[i] != NULL; i++) {
    /* If & found execute command and empty temp buffer. */
    if(strcmp(input[i], BACK) == 0) {
      execute_command(temp);
      for(j = 0; j < buffer_size; j++) {
        strcpy(temp[j], "");
      }
      j = 0;
      continue;
    }
    /* Copy values to temp buffer. */
    strcpy(temp[j], input[i]);
    j++;
  }

  /* Executes final command. */
  if(j != 0) {
    execute_command(temp);
  }
  /* Sets background back to false. */
  back = 0;
}
