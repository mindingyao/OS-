#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#define THREAD_SIZE 20
int num=0;
typedef struct {
    int value;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} sema_t;

sema_t mult_sema[THREAD_SIZE];

void sema_init(sema_t *sema, int value)
{
    sema->value = value;
    pthread_mutex_init(&sema->mutex, NULL);
    pthread_cond_init(&sema->cond, NULL);
}

void sema_wait(sema_t *sema)
{
    pthread_mutex_lock(&sema->mutex);
    sema->value--;
    while (sema->value < 0)
        pthread_cond_wait(&sema->cond, &sema->mutex);
    pthread_mutex_unlock(&sema->mutex);
}

void sema_signal(sema_t *sema)
{
    pthread_mutex_lock(&sema->mutex);
    ++sema->value;
    pthread_cond_signal(&sema->cond);
    pthread_mutex_unlock(&sema->mutex);
}

void *fthread(void *arg)
{
    int tid = *(int *)arg;
    if (tid>0)
	sema_wait(&mult_sema[tid-1]);
    printf("thread %d result=%d\n", tid, num++);
    sema_signal(&mult_sema[tid]);
    if (tid==0)
    {
	sema_wait(&mult_sema[THREAD_SIZE-1]);
	printf("thread %d result=%d\n", tid, num);
    }
}

int main()
{
    pthread_t mult_pid[THREAD_SIZE];
    int i;
    for (i=0; i<THREAD_SIZE; i++)
	sema_init(&mult_sema[i],1);
    for (i=0; i<THREAD_SIZE; i++)
	pthread_create(&mult_pid[i], NULL, fthread, &i);
    for(i = 0 ; i < THREAD_SIZE ; i++)
	pthread_join(&mult_pid[i],NULL);
    return 0;
}
