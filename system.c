#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

void mysystem(char *cmd)
{
        int pid;
	    if(cmd == NULL) return 1;
	        pid = fork();

		    int error;
		        if(pid == 0)    //child process
			        {
				            execl("/bin/sh", "sh", "-c", cmd, NULL);
					            exit(0);
						        }
}

void execute(char *comd)
{
        mysystem(comd);
	    printf(">> Command '%s' finished!\n", comd);
}

int main()
{
        execute("ls /");
	    execute("echo helloworld");
	        return 0;
}
