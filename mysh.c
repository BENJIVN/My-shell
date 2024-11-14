#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <glob.h>
#include <sys/stat.h>

#define MAX_ARGS 1024
#define MAX_LENGTH 2048

void batch_mode(const char *filename);
void interactive_mode();
int parse_and_exec();

int main(int argc, char *argv[]){
    if (argc == 2){
        //printf("Entering batch mode with file: %s\n", argv[1]);
        batch_mode(argv[1]); //file or path etc 
    } else if (isatty(STDIN_FILENO)){
        //printf("Entering interactive mode\n");
        interactive_mode();
    } else {
        return 1;
    }
    return 0;
}

void batch_mode(const char *filename){
    //printf("Batch mode not fully implemented yet. Reading from: %s\n", filename);
    int status = 0;
    char command [MAX_LENGTH];

    FILE *file = fopen(filename, "r"); //open the file in read mode 
    if(!file){
        perror("file opening error");
        exit(EXIT_FAILURE);
    }
    while(fgets(command, MAX_LENGTH, file) != NULL){
        command[strcspn(command, "\n")] = '\0'; //remove newline characters and replace with \0
        status = parse_and_exec(command, status); //execute the command
    }
    fclose(file);
}

void interactive_mode(){
    //printf("Interactive mode not fully implemented yet.\n");
    char command[MAX_LENGTH];
    int status = 0;
    int bytesRead = 0;
    int index = 0;

    write(STDOUT_FILENO, "Welcome to my shell!\n", 21);

    while(1){
    write(STDOUT_FILENO, "mysh> ", 6);
        index = 0;
         //printf("Waiting for input...\n");
        while ((bytesRead = read(STDIN_FILENO, &command[index], 1)) > 0){
            if (command[index] == '\n'){
                command[index] = '\0'; //null-terminate the character
                break;
            }
            index++;
            if (index >= MAX_LENGTH - 1){
                write(STDOUT_FILENO, "Error: command is too long\n", 27);
                index = 0; //reset the index 
                break;
            }
        }
        if (bytesRead <= 0){
            write(STDOUT_FILENO, "\n", 1);
            break;
        }
        if (strcmp(command, "exit") == 0){
            write(STDOUT_FILENO, "mysh: exiting\n", 14);
            break;
        }
        //fat fingering enter
        if (strcmp(command, "") == 0){
            continue;
        }
        status = parse_and_exec(command, status);
    }
}

int parse_and_exec(char *command, int status){
    //printf("Executing command: %s with status: %d\n", command, status);
    char *cmd = strtok(command, " ");
    char *arg1, *arg2;
    arg1 = strtok(NULL, " ");
    arg2 = strtok(NULL, " "); //checking for additional argument

    //exit command handler
    if (strcmp(command, "exit") == 0){
        write(STDOUT_FILENO, "mysh: exiting\n", 14);
        exit(0);
    }

    //cd command handler
    if (strcmp(command, "cd") == 0){
        //error checks
        if (arg1 == NULL){
            fprintf(stderr, "cd: expects one arguemnt\n");
            return -1;
        } else if (arg2 != NULL){
            fprintf(stderr, "cd: too many arguments\n");
            return -1;
        }

        //directory change
        if(chdir(arg1) != 0){
            perror("cd error");
            return -1;
        }

        return 0;
    }

    //pwd command handler
    if (strcmp(command, "pwd") == 0){
        //error check
        if(arg1 != NULL){
            fprintf(stderr, "pwd: too many arguments\n");
            return -1;
        }

        //prints the current directory path
        char cwd[MAX_LENGTH];
        if(getcwd(cwd, sizeof(cwd)) != NULL){ //gets current directory path then stores it in cwd
            printf("%s\n", cwd);
        } else {
            perror("pwd error");
        }

        return 0;
    }

    //which command handler
    if(strcmp(command, "which") == 0){
        if(arg1 == NULL || arg2 != NULL){
            fprintf(stderr, "which: expects one argument\n");
            return -1;
        }

        const char *pathSearchs[] = {"/usr/local/bin", "/usr/bin", "/bin"};
        char path[MAX_LENGTH];
        struct stat buffer;
        int found = 0;

        for(int i = 0; i < 3; i++){ //iterates through pathSearchs array
            snprintf(path, sizeof(path), "%s/%s", pathSearchs[i], arg1); //gets full path then stores it in path
            if(stat(path, &buffer) == 0 && (buffer.st_mode & S_IXUSR)){ //checks if file exists, if it does store in buffer and if user can execute the file (S_IXUSR)
                printf("%s\n", path);
                found = 1;
                break;
            }
        }

        if(!found){
            fprintf(stderr, "which: %s not found\n", arg1);
        }

        //if found is 1, returns 0
        //if found is 0, returns -1
        return found ? 0 : -1; 

    }
    return status;
}