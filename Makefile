
all: mysh
	./mysh

mysh: mysh.c mysh_func.c
	gcc -Wall -g mysh.c mysh_func.c -o mysh

clean:
	rm -f mysh
