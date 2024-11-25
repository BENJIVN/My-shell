# Authors: 
Benjamin Nguyen: bvn6

Victor Nguyen: vvn12 

# TESTPLAN:

The shell focuses on ensuring functionality, proper error handling, and memory safety (leaks) across the program. Commands were tested in both interactive and batch modes. 

Running the program:
```
make clean
make
```

To entire batch mode: 

```
./mysh _____
```

To entire interactive mode:
```
./mysh 
```
### Function prototypes:
```
//tokenizer layer / parsing 
char **tokenizer(char *command);
void wildcard_expansion(char ***tokens, int *token_count);

//command parser layer
void exec_command(char *command, int *status_flag);
void free_commands(command *cmd);
char *path_finder(const char *command);

//executor layer
void execute_pipe_commands(char **first_command, char **second_command);
void redirection(command *cmd);
int builtin_commands(char **tokens, int *status_flag, int free_tokens);
void free_expanded_tokens(char **tokens);
```
### Function Descriptions:

- char **tokenizer(char *command);
    - Splits the command string into tokens based on the whitespace and special characters. Handles wildcard expansion if present and builds the argument list in the struct for execution. 

- void wildcard_expansion(char ***tokens, int *token_count);
    - expands the tokens containing the wildcard character (*) by searching the current directory for matching filenames. Also uses glob() to create a list of matches or keeps the token if no matches are found,. 

- void exec_command(char *command, int *status_indicator);
    - parses and executes a single command or a pipeline. This function handles redirection, wildcards, built-in commands, and path using fork() and execv().

- void free_commands(command *cmd);
    - this helper function freeds dynammically allocated memory for the command struct and its arguments. 

- char *path_finder (const char * command);
    - finds the full path of an executable by searching through /usr/local/bin, /usr/bin, and /bin. Uses access() to verify the executable. 

- void execute_pipe_commands(char **first_command, char **second_command);
    - implements a 2-step process for a pipeline using pip() and dup2(). redirects the output of the first command to the stdin of the second command and uses execv()

- void redirection(command *cmd);
    - setups (<) and (>) for redirection using dup2() in the child process. Handles file creation and error reporting

- int builtin_commands(char **tokens, int *status_indicator, int free_tokens);
    - executes builtin-commands such as cd, pwd, which, exit 

- void free_expanded_tokens(char **tokens);
    - helper function to free memory allocated for token arrays 
# TESTCASES: 

## REDIRECTION: test1.txt 

opens/creates an output.txt file and writes all the files in the current working directory into it. then outputs the number of lines that are now in output.txt. shell reads the contents of inputfile.txt and writes it to outputfile.txt

```
ls > output_1.txt
wc -l < output_1.txt
cat < inputfile.txt > outputfile_1.txt
```

##  PIPING: test2.txt

prints all the contents of a text file and all directories in the current working directory. should display the # of words. the shell then lists only the files in the current directory with a .txt extension. 

```
cat 2a.txt | ls
echo hello world | wc -w
ls -l | grep .txt
```

## WILDCARDS: test3.txt 

takes all the txt files in the current working directory and sorts them and outputs it to sorted.txt. counts the number of .txt files in the current directory and redirects the list of .txt file to txtfiles.txt. combines/ concatenates all .txt files into combined.txt. lastly the shell lists all .txt files in both dsubir1 and subdir2. 

```
ls *.txt
ls *.txt | sort > sorted_3.txt
ls *.txt | wc -l
ls | grep *.txt > txtfile_3.txt
cat *.txt > combined_3.txt
ls subdir*/*.txt
```

## REIDRECION AND PIPING test4.txt

Test for of redirection, piping and wildcard expansion. The first command reads random.txt and pipes it through sort and writes the output to random_sorted_4.txt. The second command redirects inputfile.txt as input for cat and pipes the output through sort and writes the result into inputfile_sorted_4.txt. 3rd command reads a nonexistent file for error handling and pipes the output to grep to search for "text" and writes the output to output_4.txt. the last command redirects inputfile.txt to cat and pipes the output to grep using a wilcard pattern (*.txt) and writes the result to out_4.txt, a combination of everything. 

```
cat random.txt | sort > random_sorted_4.txt
cat < inputfile.txt | sort > inputfile_sorted_4.txt
cat DNE.txt | grep "text" > output_4.txt
cat < inputfile.txt | grep *.txt > out_4.txt
```

## test5.txt

a test for cd and going to subdirectories. error handling and changing proper directories. 

```
cd
cd subdir subsubdir
cd folder
cd mysh.c
cd subdir
```

## test6.txt

test for printing the current working directory. also shows error handling. pwd should not accept any arguments so we just print the current directory. 

```
pwd
pwd subdir
```

## test7.txt

test for ls and then hello world is not a built-in command. testing for the shell's ability to execute commands correctly. 

```
ls
hello world
```

## myscript.sh

simple .sh script to test the batch mode execution. should execute hello echo, but you can run any of these in batch modes as well. 

```
echo hello
```

