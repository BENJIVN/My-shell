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
void wildcard();
int exec_command();
int exec_pip_command();

int main(int argc, char *argv[]){
    if (argc == 2){
        //printf("Entering batch mode with file: %s\n", argv[1]);
        batch_mode(argv[1]);
    } else {
        //printf("Entering interactive mode\n");
        interactive_mode();
    }

    return 0;
}

void batch_mode(const char *filename){
    //printf("Batch mode not fully implemented yet. Reading from: %s\n", filename);
    int status = 0;
    char command [MAX_LENGTH];

    FILE *file = fopen(filename, "r");
    if(!file){
        perror("file opening error");
        exit(EXIT_FAILURE);
    }
    while(fgets(command, MAX_LENGTH, file) != NULL){
        command[strcspn(command, "\n")] = '\0'; //remove newline characters
        status = parse_and_exec(command, status); //execute the command
    }
    fclose(file);
}

void interactive_mode(){
    //printf("Interactive mode not fully implemented yet.\n");
    char command[MAX_LENTH];
    int status = 0;

    printf("Welcome to my shell! \n");
    while(1){
        printf("mysh> ");
    }
}

int parse_and_exec();
void wildcard();
int exec_command();
int exec_pip_command();
