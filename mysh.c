#include <stdio.h> //REMOVE
#include <unistd.h>
#include "mystring.h"
#include "jobs.h"

int main(int argc, char *argv[], char *envp[])
{
	int exit_shell = 0;
	//write(1, "mysh> ", 6);
	struct Command command;
	
	while (!exit_shell)
	{
		int error = get_command(&command);
		if (error == BUFFER_OVERFLOW)
		{
			continue;
		}
		if (error == READ_ERROR || error == EXIT)
		{
			return 1;
		}

	}
	/* TO DO: prompt for and read command line */
	/*
	while (!exit_shell)
	{
		// TO DO: process command line 
		// TO DO: prompt for and read command line 
	}
	*/
	return 0;
}