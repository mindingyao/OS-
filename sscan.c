#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <time.h>

char dirlist[1024][1024];
int curdir = 0;
int count_define = 0;

void AddDir(char *dir)
{
  //rlist[curdir++] = dir;
  strcpy(dirlist[curdir++], dir);
}
/*
int strstr1(char *str1, char *str2)
{
  if (strstr(str1, str2))
    {
  char *p = strstr(str1, str2);
  int i=0;
  while (str2[i] != '\0')
    {
      if (*p != str2[i])
	return 0;
      i++;
      p++;
    }
  return 1;
    }
  return 0;
}*/
  

void count(char *filename)
{
  FILE *rf;
  rf = fopen(filename, "r");
  char buff[1024];
  
  while(!feof(rf))
    {
      fgets(buff, sizeof(buff), rf);
      if(strstr(buff, "define"))
	{
	  count_define++;
	}
    }
  //printf("%d\n", count_define);
}

int main()
{
    int error;
    DIR *dir;
    int i=0;
    char str[1024];
    struct dirent entry;
    struct dirent *result;
    clock_t start, finish;
    
    double duration;
    start = clock();

    // AddDir("/usr/include");
    AddDir("/home/dingyao/OS/include");
    while(i < curdir)
      {
	dir = opendir(dirlist[i++]);
	for(;;)
	  {
	    error = readdir_r(dir, &entry, &result);
	    if (error != 0) {
	      perror("readdir");
	      return EXIT_FAILURE;
	    }
	    // readdir_r returns NULL in *result if the end 
	    // of the directory stream is reached
	    if (result == NULL)
	      break;
	    if (result->d_type == DT_DIR && strcmp(result->d_name, ".") && strcmp(result->d_name, ".."))
	      {
		strcpy(str,dirlist[i-1]);
		strcat(str, "/");
		strcat(str, result->d_name);
		//rlist[curdir++] = str;
		AddDir(str);
	      }
	      else if(strstr(result->d_name, ".h"))
		{
		strcpy(str,dirlist[i-1]);
		strcat(str, "/");
		strcat(str, result->d_name);
		  count(str);
		}
	  }
      }
	      
    printf("%d\n", count_define);	    		
    closedir(dir);
    finish= clock();
    duration = (double)(finish- start) / CLOCKS_PER_SEC;
    printf("time:%.4f\n", duration);
    return 0;
}
