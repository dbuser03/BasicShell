// Includes necessary libraries for the program.
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_LINE_LENGTH 4096
#define MAX_ARGUMENTS 256
#define MAX_COMMANDS 256
#define MAX_PATH_LENGTH 512
#define MAX_PROMPT_LENGTH 32

char system_path[MAX_PATH_LENGTH] = "/bin/:/usr/bin/";

void handle_error(const char* error_message) {
  if (errno) {
    fprintf(stderr, "ERROR: %s: %s \n\n", error_message, strerror(errno));
  } else {
    fprintf(stderr, "ERROR: %s \n\n", error_message);
  }
  exit(EXIT_FAILURE);
}

int get_user_input(char** buffer, const char* prompt_string) {
  if (isatty(STDIN_FILENO)) {
    *buffer = readline(prompt_string);
    if (*buffer == NULL) {
      return EOF;
    }
  } else {
    char temp_buffer[MAX_LINE_LENGTH];
    if (fgets(temp_buffer, MAX_LINE_LENGTH, stdin) == NULL) {
      return EOF;
    }
    // Remove the newline character at the end
    temp_buffer[strcspn(temp_buffer, "\n")] = 0;
    *buffer = strdup(temp_buffer);
  }
  return strlen(*buffer);
}

void update_system_path(const char* new_path) {
  if (new_path != NULL) {
    int current_position = 0;
    while (new_path[current_position] != '\0') {
      current_position++;
      if (current_position >= MAX_PATH_LENGTH - 1 &&
          new_path[current_position] != '\0') {
        fprintf(stderr, "Error: PATH string too long\n");
        return;
      }
    }
    if (current_position > 0) {
      memcpy(system_path, new_path, current_position + 1);
    }   
  }
  printf("%s\n", system_path);
}

void find_absolute_path(char* absolute_path, const char* relative_path) {
  char* path_prefix;
  char buffer[MAX_PATH_LENGTH];
  if (absolute_path == NULL || relative_path == NULL) {
    handle_error("get absolute_path: parameter error");
  }
  path_prefix = strtok(system_path, ":");
  while (path_prefix != NULL) {
    strcpy(buffer, path_prefix);
    strcat(buffer, relative_path);
    if (access(buffer, X_OK) == 0) {
      strcpy(absolute_path, buffer);
      return;
    }
    path_prefix = strtok(NULL, ":");
  }
  strcpy(absolute_path, relative_path);
}

void execute_relative_to_absolute_path(char** argument_list) {
  if (argument_list[0][0] == '/') {
    execv(argument_list[0], argument_list);
  } else {
    char absolute_path[MAX_PATH_LENGTH];
    find_absolute_path(absolute_path, argument_list[0]);
    execv(absolute_path, argument_list);
  }
}

void redirect_and_append_output(const char* output_path,
                                char** argument_list,
                                const char* mode) {
  if (output_path == NULL) {
    handle_error("redirect_and_append_output: no path");
  }
  int process_id = fork();
  if (process_id > 0) {
    int wait_process_id = wait(NULL);
    if (wait_process_id < 0) {
      handle_error("redirect_and_append_output: wait");
    }
  } else if (process_id == 0) {
    FILE* output_file = fopen(output_path, mode);
    if (output_file == NULL) {
      perror(output_path);
      exit(EXIT_FAILURE);
    }
    fseek(output_file, 0, SEEK_END);
    if (strcmp(mode, "a") == 0 && ftell(output_file) > 0) {
      fputs("----------\n", output_file);
    }
    fseek(output_file, 0, SEEK_SET);
    dup2(fileno(output_file), 1);
    execute_relative_to_absolute_path(argument_list);
    perror(argument_list[0]);
    exit(EXIT_FAILURE);
  } else {
    handle_error("redirect_and_append_output: fork");
  }
}

void redirect_output(const char* output_path, char** argument_list) {
  redirect_and_append_output(output_path, argument_list, "w");
}

// Function to append the output of a command to a file.
// It calls the redirect_and_append_output function with the mode set to "a" 
// (append).
void append_output(const char* output_path, char** argument_list) {
  redirect_and_append_output(output_path, argument_list, "a");
}

void execute_pipe(size_t pipe_position, char** argument_list) {
  int pipe_file_descriptor[2];
  int process_id;
  if (pipe(pipe_file_descriptor) < 0) {
    handle_error("execute_pipe: pipe");
  }
  process_id = fork();
  if (process_id > 0) {
    int wait_process_id = wait(NULL);
    if (wait_process_id < 0) {
      handle_error("execute_pipe: left_side: wait");
    }
  } else if (process_id == 0) {
    close(pipe_file_descriptor[0]);
    dup2(pipe_file_descriptor[1], 1);
    close(pipe_file_descriptor[1]);
    execute_relative_to_absolute_path(argument_list);
    perror(argument_list[0]);
    exit(EXIT_FAILURE);
  } else {
    handle_error("execute_pipe: left_side: fork");
  }
  process_id = fork();
  if (process_id > 0) {
    close(pipe_file_descriptor[0]);
    close(pipe_file_descriptor[1]);
    int wait_process_id = wait(NULL);
    if (wait_process_id < 0) {
      handle_error("execute_pipe: right_side: wait");
    }
  } else if (process_id == 0) {
    close(pipe_file_descriptor[1]);
    dup2(pipe_file_descriptor[0], 0);
    close(pipe_file_descriptor[0]);
    execute_relative_to_absolute_path(argument_list + pipe_position + 1);
    perror(argument_list[pipe_position + 1]);
    exit(EXIT_FAILURE);
  } else {
    handle_error("execute_pipe: right_side: fork");
  }
}

void execute_command(char** argument_list) {
  int process_id = fork();
  if (process_id > 0) {
    int status;
    waitpid(process_id, &status, 0);
  } else if (process_id == 0) {
    execute_relative_to_absolute_path(argument_list);
    perror(argument_list[0]);
    exit(EXIT_FAILURE);
  } else {
    handle_error("execute_command: fork");
  }
}

// Function to handle redirection.
void handle_redirection(char** argument_list, size_t redirect_position) {
  argument_list[redirect_position] = NULL;
  redirect_output(argument_list[redirect_position + 1], argument_list);
}

// Function to handle append.
void handle_append(char** argument_list, size_t append_position) {
  argument_list[append_position] = NULL;
  append_output(argument_list[append_position + 1], argument_list);
}

// Function to handle pipe.
void handle_pipe(char** argument_list, size_t pipe_position) {
  argument_list[pipe_position] = NULL;
  execute_pipe(pipe_position, argument_list);
}

void handle_redirection_and_execution(size_t argument_count, 
                                      char** argument_list) {
  size_t redirect_position = 0;
  size_t append_position = 0;
  size_t pipe_position = 0;
  for (size_t i = 1; i < argument_count; i++) {
    if (strcmp(argument_list[i], ">") == 0) {
      redirect_position = i;
      break;
    }
    if (strcmp(argument_list[i], ">>") == 0) {
      append_position = i;
      break;
    }
    if (strcmp(argument_list[i], "|") == 0) {
      pipe_position = i;
      break;
    }
  }
  if (redirect_position != 0) {
    handle_redirection(argument_list, redirect_position);
  }
  else if (append_position != 0) {
    handle_append(argument_list, append_position);
  }
  else if (pipe_position != 0) {
    handle_pipe(argument_list, pipe_position);
  } else {
    execute_command(argument_list);        
  }
}

void process_commands(char** command_list, size_t command_count) {
  size_t argument_count;
  for (size_t j = 0; j < command_count; j++) {
    argument_count = 0;
    char* argument_list[MAX_ARGUMENTS];
    argument_list[argument_count] = strtok(command_list[j], " ");
    if (argument_list[argument_count] == NULL) {
      continue;
    } else {
      do {
        argument_count++;
        if (argument_count > MAX_ARGUMENTS) {
          break;
        }
        argument_list[argument_count] = strtok(NULL, " ");
      } while (argument_list[argument_count] != NULL);
    }

    if (strcmp(argument_list[0], "exit") == 0) {
      puts("");
      exit(EXIT_SUCCESS);
    }
    if (strcmp(argument_list[0], "setpath") == 0) {
      update_system_path(argument_list[1]);
      continue;
    }
    if (strcmp(argument_list[0], "clear") == 0) {
      printf("\033[H\033[J");
      continue;
    }
    handle_redirection_and_execution(argument_count, argument_list);
  }
}

int main(void) {
  char* input_buffer;
  char* command_list[MAX_COMMANDS];
  char prompt_string[MAX_PROMPT_LENGTH] = "\0";
  if (isatty(0)) {
    strcpy(prompt_string, "dsh$ \0");
  }
  while (get_user_input(&input_buffer, prompt_string) >= 0) {
    if (*input_buffer) {
      if (history_length >= 10) {
        remove_history(0);
      }
      add_history(input_buffer);
    }
    size_t command_count = 0;
    command_list[command_count] = strtok(input_buffer, ";");
    if (command_list[command_count] == NULL) {
      free(input_buffer);
      continue;
    } else {
      do {
        command_count++;
        if (command_count > MAX_COMMANDS) {
          break;
        }
        command_list[command_count] = strtok(NULL, ";");
      } while (command_list[command_count] != NULL);
    }
    process_commands(command_list, command_count);
    free(input_buffer);
  }
  puts("");
  exit(EXIT_SUCCESS);
}
