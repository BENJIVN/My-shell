CC = gcc
CFLAGS = -Wall -Wextra -g -fsanitize=address,undefined -DREALMALLOC -Wvla

mysh: mysh.o 
	$(CC) $(CFLAGS) -o mysh mysh.o 

mysh.o: mysh.c
	$(CC) $(CFLAGS) -c mysh.c

clean:
	rm -f mysh mysh.o 