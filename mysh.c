#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <glob.h>
#include <sys/stat.h>
#include <ctype.h>

#define MAX_ARGS 512
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

/*
determine if the shell is running in batch or interactive mode 
reads commands from stdin or a file in batch mode and executes them using exec_command
*/
int main (int argc, char *argv[]){
    int interactive = isatty(STDIN_FILENO); //check stdin is connected to terminal 
    char command[MAX_COMMAND_LENGTH];
    ssize_t bytes_read;
    //isatty() returns 1 is true (shell is connected to the terminal)
    if (argc == 1 && interactive){
        write(STDERR_FILENO, "Welcome to my shell!\n",21);

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
    } else if (argc == 1 && !interactive){  //batch mode with piped input (cat myscript.sh | ./mysh)
        while ((bytes_read = read(STDIN_FILENO, command, MAX_COMMAND_LENGTH - 1)) > 0) {
            command[bytes_read] = '\0';  
            char *next_comm = command;

            while (next_comm) {
                char *newLine = strchr(next_comm, '\n');  
                if (newLine) {
                    *newLine = '\0';
                }

                if (strlen(next_comm) > 0) {
                    exec_command(next_comm, &status);
                }

                //move to next line if not null, end loop if no new lines
                next_comm = newLine ? newLine + 1 : NULL;
            }
        }

        if (bytes_read == -1) { 
            perror("read");
            exit(EXIT_FAILURE);
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

/*
parses a given command into tokens for exec 
splits the command string into tokens 
allocates memory for tokens 
--> wildcard_expansion to handle * expansions in the tokens 
returns an array of tokens 
*/
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
        //skip whitespace 
        while (isspace(*next_token)) next_token++;
            if(*next_token == '\0') break;
        
        //special tokens 
        if (*next_token == '>' || *next_token == '<' || *next_token == '|' ){
            tokens[token_count++] = strndup(next_token,1);
            next_token++;
        } else {
            //parse nonspecial tokens 
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

/*
expands tokens that have * 
iterates through each token 
if a token contains a wildcard, uses glob function to find matching file names
replaces the token with the list of the file names that correspond to the *...
*/
void wildcard_expansion(char ***tokens, int *token_count) {
    char **expanded_tokens = malloc(MAX_ARGS * sizeof(char *));
    if (!expanded_tokens) {
        perror("Failed to allocate memory for wildcard expansion");
        exit(EXIT_FAILURE);
    }

    int new_token_count = 0;

    for (int i = 0; i < *token_count; i++) {
        if (strchr((*tokens)[i], '*')) {
            glob_t glob_result;
            if (glob((*tokens)[i], 0, NULL, &glob_result) == 0) {
                //fprintf(stderr, "Wildcard Expansion: %s ->", (*tokens)[i]);
                for (size_t j = 0; j < glob_result.gl_pathc; j++) {
                    expanded_tokens[new_token_count++] = strdup(glob_result.gl_pathv[j]);
                }

                //fprintf(stderr, "Wildcard Expansion: %s ->", (*tokens)[i]);
                // for (size_t j = 0; j < glob_result.gl_pathc; j++) {
                //     fprintf(stderr, " %s", glob_result.gl_pathv[j]);
                // }

                fprintf(stderr, "\n");
                globfree(&glob_result);
            } else {
                expanded_tokens[new_token_count++] = strdup((*tokens)[i]);
            }
        } else {
            expanded_tokens[new_token_count++] = strdup((*tokens)[i]);
        }
        free((*tokens)[i]);
    }

    free(*tokens);

    expanded_tokens[new_token_count] = NULL;
    *tokens = expanded_tokens;
    *token_count = new_token_count;
}

/*
executes a single command / pipeline 
parses the command string into tokens using parse
checks if the command is a built-in command 
handles input and output redirection (< , >)
if the command is not built in we find its path using path_finder 
also remove the redirection from the arguments array 
*/
void exec_command(char *input, int *status) {
    //we need to handle the pipe as well
    char *pipe = strchr(input, '|');
    if (pipe){
        *pipe = '\0';
        char *first_comm = input;
        char *second_comm = pipe + 1;

        execute_pipe_commands(parse_command(first_comm), parse_command(second_comm));

        // for (int i = 0; first_tokens[i] != NULL; i++) free(first_tokens[i]);
        // free(first_tokens);

        // for (int i = 0; second_tokens[i] != NULL; i++) free(second_tokens[i]);
        // free(second_tokens);

        return;
    }

    char **tokens = parse_command(input);
    if (!tokens || tokens[0] == NULL) {
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
    if (strcmp(tokens[i], "<") == 0 && tokens[i + 1] != NULL) {
        cmd.inputfile = strdup(tokens[++i]);  
        tokens[i - 1] = NULL;                
    } else if (strcmp(tokens[i], ">") == 0 && tokens[i + 1] != NULL) {
        cmd.outputfile = strdup(tokens[++i]);  
        tokens[i - 1] = NULL;                  
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
    } else {
        redirection(&cmd);
    }
    
    //redirection(&cmd);
    
    free(cmd.execpath);
    free(cmd.inputfile);
    free(cmd.outputfile);

    for (int i = 0; tokens[i] != NULL; i++) {
        free(tokens[i]);
    }

    free(tokens);
}

/*
input/output redirection 
forks a child process to execute the command 
in the child process: set up input redirection if an input file is specified same with output redirection
executes the command using execv 
parent: waits forr child process to finish 
*/
void redirection(command *cmd) {
    pid_t pid = fork();
    if (pid == 0) { // child process
        //input redirection
        if (cmd->inputfile) {
            int fd_in = open(cmd->inputfile, O_RDONLY);
            if (fd_in == -1) {
                perror("Failed to open input file");
                exit(EXIT_FAILURE);
            }
            // dup2(fd_in, STDIN_FILENO);
            // close(fd_in);
            if (dup2(fd_in, STDIN_FILENO) == -1) {
                perror("Failed to redirect input");
                close(fd_in);
                exit(EXIT_FAILURE);
            }
            close(fd_in);
        }
        if (cmd->outputfile) {
            int fd_out = open(cmd->outputfile, O_WRONLY | O_CREAT | O_TRUNC, 0640);
            if (fd_out == -1) {
                perror("Failed to open output file");
                exit(EXIT_FAILURE);
            }
            // 
            if (dup2(fd_out, STDOUT_FILENO) == -1) {
                perror("Failed to redirect output");
                close(fd_out);
                exit(EXIT_FAILURE);
            }
            close(fd_out);
        }
        
        execv(cmd->execpath, cmd->arguments);
        perror("execv failed");
        exit(EXIT_FAILURE);
    } else if (pid > 0) { //parent 
        int wstatus;
        waitpid(pid, &wstatus, 0);

        //check status of child process
        if (WIFEXITED(wstatus)) { 
            int exit_code = WEXITSTATUS(wstatus);
            if (exit_code != 0) {
                fprintf(stderr, "mysh: Command failed with code %d\n", exit_code);
            }
        } else if (WIFSIGNALED(wstatus)) {
            fprintf(stderr, "mysh: Terminated by signal %d\n", WTERMSIG(wstatus));
        }
    } else {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
}

/*
finds the path of an command that is not built in 
checks if the command contains a /
checks directories in the PATH env 
*/
char *path_finder(const char *command){
    //fprintf(stderr, "debug: %s\n", command);
    char *path_env = getenv("PATH");
    if (path_env == NULL) {
        perror("PATH environment variable not found");
        return NULL;
    }

    if (strchr(command, '/')) {
        if (access(command, X_OK) == 0) return strdup(command);
        return NULL;
    }

    const char *paths[] = {"/usr/local/bin", "/usr/bin", "/bin", NULL};
    char full_path[MAX_COMMAND_LENGTH];
    for (int i = 0; paths[i]; i++) {
        snprintf(full_path, sizeof(full_path), "%s/%s", paths[i], command);
        if (access(full_path, X_OK) == 0) {
        return strdup(full_path);
        }
    }
    return NULL;
}

/*
built-in shell commands: cd, exit, pwd, which 
*/
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
        char cwd[MAX_COMMAND_LENGTH];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
            *status = 0;
        } else {
            perror("pwd failed");
            *status = 1;
        }
        return 1;
        // if (tokens[1] != NULL) {
        //     fprintf(stderr, "pwd: too many arguments\n");
        //     *status = 1;
        // } else {
        //     char cwd[MAX_COMMAND_LENGTH];
        //     if (getcwd(cwd, sizeof(cwd)) != NULL) {
        //         printf("%s\n", cwd);
        //         *status = 0;
        //     } else {
        //         perror("pwd failed");
        //         *status = 1;
        //     }
        // }
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
    return 0; 
}


/*
executes a pipeline of 2 commands that are connected using | 
forks 2 child processes 
1st process writes to the pipe
2nd process reads from the pipe 
execv is used in the child process 
*/
void execute_pipe_commands (char **first_command, char **second_command){
   int pipe_fds[2];
    if (pipe(pipe_fds) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t first_pid = fork();
    if (first_pid == 0) {
        //write to pipe
        dup2(pipe_fds[1], STDOUT_FILENO);
        close(pipe_fds[0]);
        close(pipe_fds[1]);

        command cmd1 = { .arguments = first_command, .execpath = path_finder(first_command[0]), .inputfile = NULL, .outputfile = NULL };

        // Check for input redirection in first command
        for (int i = 0; first_command[i] != NULL; i++) {
            if (strcmp(first_command[i], "<") == 0 && first_command[i + 1] != NULL) {
                cmd1.inputfile = strdup(first_command[++i]);
                first_command[i - 1] = NULL;
                break;
            }
        }
        if (cmd1.inputfile) {
            int fd_in = open(cmd1.inputfile, O_RDONLY);
            if (fd_in == -1) {
                perror("Failed to open input file");
                //free(cmd1.inputfile); 
                exit(EXIT_FAILURE);
            }
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
        }
        execv(cmd1.execpath, cmd1.arguments);
        perror("execv failed for first command");
        //free(cmd1.inputfile);
        exit(EXIT_FAILURE);
    }
    pid_t second_pid = fork();
    if (second_pid == 0) {
        //read from pipe
        dup2(pipe_fds[0], STDIN_FILENO);
        close(pipe_fds[1]);
        close(pipe_fds[0]);

        command cmd2 = { .arguments = second_command, .execpath = path_finder(second_command[0]), .inputfile = NULL, .outputfile = NULL };

        // Check for output redirection in second command
        for (int i = 0; second_command[i] != NULL; i++) {
            if (strcmp(second_command[i], ">") == 0 && second_command[i + 1] != NULL) {
                cmd2.outputfile = strdup(second_command[++i]);
                second_command[i - 1] = NULL;
                break;
            }
        }

        if (cmd2.outputfile) {
            int fd_out = open(cmd2.outputfile, O_WRONLY | O_CREAT | O_TRUNC, 0640);
            if (fd_out == -1) {
                perror("Failed to open output file");
                //free(cmd2.outputfile);
                exit(EXIT_FAILURE);
            }
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        }

        execv(cmd2.execpath, cmd2.arguments);
        perror("execv failed for second command");
        //free(cmd2.outputfile);
        exit(EXIT_FAILURE);
    }

    close(pipe_fds[0]);
    close(pipe_fds[1]);
    waitpid(first_pid, NULL, 0);
    waitpid(second_pid, NULL, 0);

    for (int i = 0; first_command[i] != NULL; i++) free(first_command[i]);
    free(first_command);

    for (int i = 0; second_command[i] != NULL; i++) free(second_command[i]);
    free(second_command);
}