#include <stdio.h>
#include <unistd.h>
#include <memory.h>
#include <sys/wait.h>
#include <stdlib.h>

void execute(char *cmd)
{
    int pid;
    pid = fork();
    int error;
    if (pid == 0)
    {
	error = execl("/bin/sh", "sh", "-c", cmd, NULL);
	if (error == -1)
	{
	    printf("'%s' commmend is not found.\n", cmd);
	    exit(0);
	}
    }

    waitpid(pid, NULL, 0);
}

int commend(char *cmd, char **argv)
{
    char *pch;
    if (cmd == NULL)
	return -1;
    pch = strtok(cmd, " ");
    int i = 0;
    while (pch != NULL)
    {
	argv[i++] = pch;
	pch = strtok(NULL, " ");
    }

    if (strcmp(argv[0], "exit") == 0)
	exit(0);
    if (strcmp(argv[0], "cd") == 0)
    {
	if (chdir(argv[1])<0)
	{
	    perror("chdir failed!\n");
	    return 0;
	}

	return 1;
    }
    if (strcmp(argv[0], "pwd") == 0)
    {
	char buf[512];
	getcwd(buf, sizeof(buf));
	printf("%s\n", buf);
	return 1;
    }

    return 0;
}

int main()
{
    char *argv[512];
    char buf[512];
    char c;
    char cmd[512];
    int n;
    FILE *shell;
    char pwd[512];

    while(1)
    {
	n = 0;
	shell = popen("/bin/pwd", "r");
	fscanf(shell, "%s", pwd);
	pclose(shell);
	printf("$%s>", pwd);
   	 while(1)
   	 {	
		scanf("%c", &c);
		if (c == '\n')
		    break;
		cmd[n++] = c;
   	 }

   	 memset(buf, 0, sizeof(buf));
   	 strcpy(buf, cmd);
   	 if (!commend(buf, argv))
	      execute(cmd);
   	 memset(cmd, 0, sizeof(cmd));
    }
}
