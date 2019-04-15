#include <stdio.h>
#include <unistd.h>
#include <memory.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>


							     	

void execute(char *cmd, char **argv)
{
    int pid;
    pid = fork();
    int error;
    if (pid == 0)
    {
	if (strcmp(argv[0], "echo") == 0)
	{
	    int i=1;
	    while(argv[i]) i++;
	    i--;
	    char buf[512];
	    strcpy(buf, argv[i]);
	    int flag = 0;
	    if (strcmp(argv[i-1], ">") == 0)
	    {
		flag = 1;
	    }
	    if (flag == 1)
	    {
		int fd;
		//printf("1\n");
		fd = open("log",  O_CREAT|O_RDWR, 0666);
       		//printf("1\n");
		dup2(fd, 1);
		close(fd);
		//printf("%s\n",argv[1]);
		int l = strlen(argv[1]);
		write(1, argv[1], l);
		return;
	    }
	}
	   else
	   {
	       error = execl("/bin/sh", "sh", "-c", cmd, NULL);
	        if (error == -1)
		  {
                  printf("'%s' commmend is not found.\n", cmd); 
	          exit(0);
	          
		 }
		return;
	   }
    }
   // waitpid(pid, NULL, 0);
}
/*
void mysys(char **argv)
{
    pid_t pid;
    char tmp[512], t;
    int err, fd[2], file, flag = 0,i=1;
    if (strcmp(argv[0], "echo") == 0)
    {
	while (argv[i])
	    i++;
	i--;
	if (strcmp(argv[i-2], ">") == 0)
	{
	    flag = 1;
	    strcpy(tmp, argv[i-1]);
	}
    }
    pid = fork();
    if (pid == 0)
    {
	if (strcmp(argv[i-2], ">") == 0)
	{
	    pid_t ppid;
	    pipe(fd);
	    ppid = fork();
	    if (ppid == 0)
	    {
		dup2(fd[1], 1);
		close(fd[0]);
		close(fd[1]);
		argv[i-2] = NULL;
		argv[i-1] = NULL;
		err = execvp(argv[0], argv);
		if (err < 0)
		    perror("execvp");
	    }
	    else
	    {
		wait(NULL);
		if (flag)
		{
		    dup2(fd[0], 0);
		    close(fd[0]);
		    close(fd[1]);
		    file = open(tmp, O_RDWR|O_CREAT,0666);
		    while(1)
		    {
			if (!read(0, &t, sizeof(char)))
			    break;
			write(file, &t, sizeof(char));
		    }
		}
	    }
	    return;
	}
	else
	{
	    err = execvp(argv[0], argv);
	    if (err < 0)
		perror("execvp");
	}
   }
   else
   {
       wait(NULL);
   }
   return;
}
*/

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
    while(1)
    {
	n = 0;
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
	execute(cmd,argv);
// 	mysys(argv);
    memset(argv, 0, sizeof(argv));
    }
    return 0;
    
}
