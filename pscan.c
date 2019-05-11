#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <time.h>


typedef struct task{
    void * (*pooltask) (void *args); 
    void * args;
    struct task * next;
}task_t;

typedef struct threadpool{
    pthread_cond_t ready;       //条件变量控制线程池中的线程处理任务
    pthread_mutex_t lock;       //控制并发的任务获取
    pthread_t *ptids;           //线程id列表
    task_t * first;             //任务头指针
    task_t * last;              //任务末尾指针
    int task_count;             //任务数目
    int max_threads;            //线程池的最大线程数目
    int quit;                   //线程池结束标志
}threadpool_t;

threadpool_t *pool=NULL;
void pool_init(int max_threads);
int pool_add_task(void * (*pooltask)(void *args),void *args);
int pool_destroy();
void *pool_thread_loop(void *args);
pthread_cond_t q_empty;

void pool_init(int max_threads)
{
	pool = (threadpool_t *)malloc(sizeof(threadpool_t));

	pthread_mutex_init(&pool->lock, NULL);
	pthread_cond_init(&pool->ready, NULL);
	pthread_cond_init(&q_empty, NULL);

	pool->task_count = 0;
	pool->max_threads = max_threads;
	pool->first = NULL;
	pool->last = pool->first;
	pool->quit = 0;
	pool->ptids = (pthread_t *)malloc(sizeof(pthread_t)*max_threads);
	int i;
	for(i=0;i<max_threads;i++){
        pthread_create(&(pool->ptids[i]),NULL,pool_thread_loop,NULL);//创建线程池的线程
    }
}

int pool_add_task(void * (*pooltask)(void *args),void *args)
{
	task_t *task = (task_t *)malloc(sizeof(task_t));
	char *dir = (char *)malloc(sizeof(char)*1024);
	strcpy(dir, (char *)args);
	task->pooltask = pooltask;
	task->args = (void *)dir;
	task->next = NULL;
	pthread_mutex_lock(&pool->lock);
	if (pool->first == NULL)
	{
		pool->first = task;
		pool->last = task;
	}
	else
	{
		pool->last->next = task;
		pool->last = task;
	}
	pool->task_count ++;
	pthread_cond_signal(&pool->ready);
	pthread_mutex_unlock(&pool->lock);
	return 0;
}

int pool_destroy()
{
	pool->quit = 1; //退出
	 /*唤醒所有等待线程，线程池要销毁了*/
	pthread_cond_broadcast(&pool->ready);
	int i;
	for (i-0; i<pool->max_threads; i++)
	{
		pthread_join(pool->ptids[i], NULL);//等待线程结束
	}
	free(pool->ptids);
	task_t *ftask;
	while(pool->first)
	{
		ftask = pool->first;
		pool->first = ftask->next;
		free(ftask);
	}
	pthread_mutex_destroy(&pool->lock);
	pthread_cond_destroy(&pool->ready);
	free(pool);
	pool = NULL;
	return 0;
}

void *pool_thread_loop(void *args)
{
	while (1)
	{
		pthread_mutex_lock(&pool->lock);
		while(pool->task_count == 0 && pool->quit == 0)
		{
			pthread_cond_wait(&pool->ready, &pool->lock);
		}
		if(pool->quit){        //判断是否结束线程池
            pthread_mutex_unlock(&pool->lock);
            pthread_exit(NULL);
        }
        task_t *ttask;
        ttask = pool->first;
        pool->first = pool->first->next;
        pool->task_count --;
        pthread_mutex_unlock(&pool->lock);

        /*调用回调函数，执行任务*/
        (*(ttask->pooltask))(ttask->args);
        free((char *)ttask->args);
	pthread_mutex_lock(&pool->lock);
        if(pool->task_count == 0){
            pthread_cond_signal(&q_empty);  //通知主线程执行结束
        }
        pthread_mutex_unlock(&pool->lock);
	}
}

char dirlist[1024][1024];
int curdir = 0;
int count_define = 0;

void AddDir(char *dir)
{
  strcpy(dirlist[curdir++], dir);
}

  

void count(char *filename)
{
  FILE *rf;
  printf("%s\n", filename);
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
    pool_init(6);
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
		pool_add_task((void *)count, (void *)str);
		}
	  }
      }
    /*等待所有任务完成*/
    // sleep (5);
    pthread_mutex_lock(&pool->lock);
    while(pool->task_count > 0){
        pthread_cond_wait(&q_empty,&pool->lock);   //等待线程池结束
    }
    pthread_mutex_unlock(&pool->lock);
    /*销毁线程池*/
    pool_destroy ();
	      
    printf("%d\n", count_define);	    		
    closedir(dir);
    
    finish= clock();
    duration = (double)(finish- start) / CLOCKS_PER_SEC;
    printf("time:%.4f\n", duration);
    return 0;
}

