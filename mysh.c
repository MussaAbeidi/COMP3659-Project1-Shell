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
		// getting the command from the user
		int status = get_command(&command);

		// if there is a buffer overflow, we will continue to the next iteration of the loop
		if (status == BUFFER_OVERFLOW)
		{
			continue;
		}
		// if there is a read error, we will exit the shell
		if (status == READ_ERROR)
		{
			return 1;
		}
		// if the user types "exit", we will exit the shell
		if (status == EXIT)
		{
			return 0;
		}
		// executing the command
		run_command(&command);
	}
	return 0;
}