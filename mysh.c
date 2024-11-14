#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <glob.h>

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

    printf("Welcome to my shell! \n");

    while(1){
        printf("mysh> ");
        index = 0;
         //printf("Waiting for input...\n");
        while ((bytesRead = read(STDIN_FILENO, &command[index], 1)) > 0){
            if (command[index] == '\n'){
                command[index] = '\0'; //null-terminate the character
                break;
            }
            index++;
            if (index >= MAX_LENGTH - 1){
                fprintf(stderr, "Error: command is too long\n");
                index = 0; //reset the index 
                break;
            }
        }
        if (bytesRead <= 0){
            break;
        }
        if (strcmp(command, "exit") == 0){
            printf("exiting\n");
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
    printf("Executing command: %s with status: %d\n", command, status);
    return 0;
}

