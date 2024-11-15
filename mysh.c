#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <glob.h>
#include <sys/stat.h>
#include <ctype.h>

#define MAX_ARGS 1024
#define MAX_COMMAND_LENGTH 2048
int status = 0;

typedef struct {
    char **arguments;   
    char *execpath;     
    char *inputfile;    
    char *outputfile;   
} command;

//function prototypes!!!
//tokenizer layer
char **parse_command(char *command);
void wildcard_expansion(char ***tokens, int *token_count);

//command parser layer
void exec_command(char *command, int *status_flag);
char *path_finder(const char *command);

//executor layer
void execute_pipe_commands(char **first_command, char **second_command);
void redirection(command *cmd);
int builtin_commands(char **tokens, int *status_flag);

int main (int argc, char *argv[]){

    int interactive = isatty(STDIN_FILENO); //check stdin is connected to terminal 
    //isatty() returns 1 is true (shell is connected to the terminal)
    if (argc == 1){
        if (interactive){
            write(STDERR_FILENO, "Welcome to my shell!\n",21);
            char command[MAX_COMMAND_LENGTH];
            ssize_t bytes_read;

            while(interactive){
                write(STDOUT_FILENO, "mysh> ", 6);
                bytes_read = read(STDIN_FILENO, command, MAX_COMMAND_LENGTH-1);
                if(bytes_read == -1){
                    perror("read");
                    exit(EXIT_FAILURE);
                }
                if(bytes_read == 0){
                    break;
                }
                if (command[bytes_read - 1] == '\n'){
                    command[bytes_read - 1] = '\0'; //null terminator at the end 
                }

                if (strlen(command) > 0){
                    exec_command(command, &status);
                }
            }
        }
    } else if (argc > 1){
        int fd = open(argv[1], O_RDONLY);
        if (fd == -1){
            fprintf(stderr, "Error opening file: %s\n", argv[1]);
            exit(EXIT_FAILURE);
        }
        char command[MAX_COMMAND_LENGTH];
        ssize_t bytes_read;

        while ((bytes_read = read(fd, command, MAX_COMMAND_LENGTH - 1)) > 0){
            command[bytes_read] = '\0';

            char *next_comm = command;
            while (next_comm){
                char *newLine = strchr(next_comm, '\n');//searches for newlines and breaks them up with a null terminator
                if (newLine){
                    *newLine = '\0';
                }

                if (strlen(next_comm) > 0){ 
                     exec_command(command, &status);
                }

                if (newLine){
                    next_comm = newLine + 1;
                } else {
                    break;
                }
            }
        } if (bytes_read == -1){
            perror("read");
            close(fd);
            exit(EXIT_FAILURE);
        }
        close(fd);
    }
}

char **parse_command(char *command){
    int token_count = 0;
    char **tokens = malloc(MAX_ARGS * sizeof(char *));
    char *next_token = command;
    char *end_token;

    if (!tokens){
        perror("memory allocation failed");
        exit(EXIT_FAILURE);
    }

    while (*next_token != '\0'){
        while (isspace(*next_token)) next_token++;
            if(*next_token == '\0') break;
        
        if (*next_token == '>' || *next_token == '<' || *next_token == '|' ){
            tokens[token_count++] = strndup(next_token,1);
            next_token++;
        } else {
            end_token = next_token;
            while (!isspace(*end_token) && *end_token != '\0' && *end_token != '>' && *end_token != '<' && *end_token != '|') end_token++;
            tokens[token_count++] = strndup(next_token, end_token - next_token);
            next_token = end_token;
        }

        if (token_count >= MAX_ARGS - 1){
            fprintf(stderr, "too many arguments\n");
            free(tokens);
            exit(EXIT_FAILURE);
        }
    }

    tokens[token_count] = NULL;

    //debug tokens
    // for (int i = 0; i < token_count; i++) {
    //     fprintf(stderr, "parse_command Debug: Token %d: %s\n", i, tokens[i]);
    // }

    wildcard_expansion(&tokens, &token_count);
    return tokens;
}

void wildcard_expansion(char ***tokens, int *token_count) {
    char **expanded_tokens = malloc(MAX_ARGS * sizeof(char *));
    int new_token_count = 0;

    for (int i = 0; i < *token_count; i++) {
        if (strchr((*tokens)[i], '*')) {
            glob_t glob_result;
            if (glob((*tokens)[i], 0, NULL, &glob_result) == 0) {
                for (size_t j = 0; j < glob_result.gl_pathc; j++) {
                    expanded_tokens[new_token_count++] = strdup(glob_result.gl_pathv[j]);
                }
                globfree(&glob_result);
            }
        } else {
            expanded_tokens[new_token_count++] = strdup((*tokens)[i]);
        }
    }
    expanded_tokens[new_token_count] = NULL;
    for (int i = 0; i < *token_count; i++) free((*tokens)[i]);
    free(*tokens);
    *tokens = expanded_tokens;
    *token_count = new_token_count;
}

void exec_command(char *input, int *status) {
    char **tokens = parse_command(input);
    if (tokens[0] == NULL) {
        free(tokens);
        return;
    }

    if (builtin_commands(tokens, status)) {
        for (int i = 0; tokens[i] != NULL; i++) free(tokens[i]);
        free(tokens);
        return;
    }

    command cmd = { .arguments = tokens, .execpath = NULL, .inputfile = NULL, .outputfile = NULL };

    for (int i = 0; tokens[i] != NULL; i++) {
        if (strcmp(tokens[i], "<") == 0 && tokens[i+1] != NULL) {
            cmd.inputfile = strdup(tokens[++i]);
        } else if (strcmp(tokens[i], ">") == 0 && tokens[i+1] != NULL) {
            cmd.outputfile = strdup(tokens[++i]);
        } else if (cmd.execpath == NULL) {
            cmd.execpath = path_finder(tokens[0]);
        }
    }

    if (!cmd.execpath) {
        fprintf(stderr, "Command not found: %s\n", tokens[0]);
        *status = 1;
        for (int i = 0; tokens[i] != NULL; i++) free(tokens[i]);
        free(tokens);
        return;
    }
    
    redirection(&cmd);
    
    free(cmd.execpath);
    free(cmd.inputfile);
    free(cmd.outputfile);
    for (int i = 0; tokens[i] != NULL; i++) free(tokens[i]);
    free(tokens);
}

void redirection(command *cmd) {
    pid_t pid = fork();
    if (pid == 0) { // child process
        if (cmd->inputfile) {
            int fd_in = open(cmd->inputfile, O_RDONLY);
            if (fd_in == -1) {
                perror("Failed to open input file");
                exit(EXIT_FAILURE);
            }
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
        }
        if (cmd->outputfile) {
            int fd_out = open(cmd->outputfile, O_WRONLY | O_CREAT | O_TRUNC, 0640);
            if (fd_out == -1) {
                perror("Failed to open output file");
                exit(EXIT_FAILURE);
            }
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        }
        execv(cmd->execpath, cmd->arguments);
        perror("execv failed");
        exit(EXIT_FAILURE);
    } else if (pid > 0) { // Parent process
        waitpid(pid, NULL, 0);
    } else {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
}

char *path_finder(const char *command){
    //fprintf(stderr, "Debug: Checking path for command: %s\n", command);
    char *path_env = getenv("PATH");
    if (path_env == NULL) {
        perror("PATH environment variable not found");
        return NULL;
    }

    char *path = strdup("PATH");
    char *dir = strtok(path, ":");
    char full_path[MAX_COMMAND_LENGTH];

    while (dir != NULL){
        snprintf(full_path, sizeof(full_path), "%s/%s", dir, command);
        if (access(full_path, X_OK) == 0){
            //fprintf(stderr, "Debug: Found command at: %s\n", full_path);
            free(path);
            return strdup(full_path); //return the path 
        }
        dir = strtok(NULL, ":");
    }
    //fprintf(stderr, "Debug: Command not found: %s\n", command);
    free(path);
    return NULL; //command not found 
}

int builtin_commands (char **tokens, int *status){
    if (strcmp(tokens[0], "exit") == 0) {
        printf("mysh: exiting");
        printf("\n");
        exit(0); // Exit the shell
    } else if (strcmp(tokens[0], "cd") == 0) {
        if (tokens[1] == NULL || tokens[2] != NULL) {
            fprintf(stderr, "cd: wrong number of arguments\n");
            *status = 1;
        } else if (chdir(tokens[1]) != 0) {
            perror("cd");
            *status = 1;
        } else {
            *status = 0;
        }
        return 1;
    } else if (strcmp(tokens[0], "pwd") == 0) {
        if (tokens[1] != NULL) {
            fprintf(stderr, "pwd: too many arguments\n");
            *status = 1;
        } else {
            char cwd[MAX_COMMAND_LENGTH];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("%s\n", cwd);
                *status = 0;
            } else {
                perror("pwd failed");
                *status = 1;
            }
        }
        return 1;
    } else if (strcmp(tokens[0], "which") == 0) {
        if (tokens[1] == NULL || tokens[2] != NULL) {
            fprintf(stderr, "which: wrong number of arguments\n");
            *status = 1;
        } else {
            char *path = path_finder(tokens[1]);
            if (path) {
                printf("%s\n", path);
                free(path);
                *status = 0;
            } else {
                *status = 1;
            }
        }
        return 1;
    }
    return 0; // Not a built-in command
}

void execute_pipe_commands (char **first_command, char **second_command){
    int pipe_fds[2];
    if (pipe(pipe_fds) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t first_pid = fork();
    if (first_pid == 0) {
        // First child --> execute the first command
        dup2(pipe_fds[1], STDOUT_FILENO); 
        close(pipe_fds[0]);
        close(pipe_fds[1]);

        command command1;
        command1.arguments = first_command;
        command1.execpath = path_finder(first_command[0]);
        if (command1.execpath) {
            execv(command1.execpath, command1.arguments);
            perror("execv failed for first command");
        }
        exit(EXIT_FAILURE);
    }

    pid_t second_pid = fork();
    if (second_pid == 0) {
        // Second child: execute the second command
        dup2(pipe_fds[0], STDIN_FILENO);
        close(pipe_fds[1]);
        close(pipe_fds[0]);
        command command2;
        command2.arguments = second_command;
        command2.execpath = path_finder(second_command[0]);
        if (command2.execpath) {
            execv(command2.execpath, command2.arguments);
            perror("execv failed for second command");
        }
        exit(EXIT_FAILURE);
    }
    // Parent process closes pipes and waits for children
    close(pipe_fds[0]);
    close(pipe_fds[1]);
    waitpid(first_pid, NULL, 0);
    waitpid(second_pid, NULL, 0);
}