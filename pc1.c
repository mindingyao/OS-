#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<unistd.h>

#define cap 4
#define count 8

char buffer[3][cap];
int in[3] = {0,0,0};
int out[3] = {0,0,0};

int buffer_is_empty(int num)
{
    return in[num] == out[num];
}

int buffer_is_full(int num)
{
    return (in[num]+1)%cap == out[num];
}

char get_item(int num)
{
    char item;

    item = buffer[num][out[num]];
    out[num] = (out[num] + 1)%cap;
    return item;
}

void put_item(int num, char item)
{
    buffer[num][in[num]] = item;
    in[num] = (in[num]+1)%cap;
}

pthread_mutex_t mutex1, mutex2;
pthread_cond_t wait_empty_buffer1, wait_empty_buffer2;
pthread_cond_t wait_full_buffer1, wait_full_buffer2;

void *consume(void *arg)
{
    int i;
    char item;

    for (i=0; i<count; i++)
    {
	pthread_mutex_lock(&mutex2);
	while(buffer_is_empty(2))
	{
	    pthread_cond_wait(&wait_full_buffer2, &mutex2);
	}

	item = get_item(2);
	printf("    consume item: %c\n", item);

	pthread_cond_signal(&wait_empty_buffer2);
	pthread_mutex_unlock(&mutex2);
    }
    return NULL;
}

void *calculate(void *arg)
{
    int i;
    char item;
    for (i=0; i<count; i++)
    {
	pthread_mutex_lock(&mutex1);
	while(buffer_is_empty(1))
	{
	    pthread_cond_wait(&wait_full_buffer1, &mutex1);
	}
	item = get_item(1);
	
	pthread_cond_signal(&wait_empty_buffer1);
	pthread_mutex_unlock(&mutex1);


	pthread_mutex_lock(&mutex2);
	while(buffer_is_full(2))
	    pthread_cond_wait(&wait_empty_buffer2, &mutex2);
	put_item(2, toupper(item));
	printf("	calculate item: %c\n",toupper(item));

	pthread_cond_signal(&wait_full_buffer2);
	pthread_mutex_unlock(&mutex2);
    }
    return NULL;
}

void *produce(void *arg)
{
    int i;
    char item;
    for(i=0; i<count; i++)
    {
	pthread_mutex_lock(&mutex1);
	while(buffer_is_full(1))
	    pthread_cond_wait(&wait_empty_buffer1, &mutex1);

	item = 'a'+i;
	printf("produce item: %c\n", item);
	put_item(1, item);

	pthread_cond_signal(&wait_full_buffer1);
	pthread_mutex_unlock(&mutex1);
    }
    return NULL;
}

int main()
{
    pthread_t tid1,tid2,tid3;
    pthread_mutex_init(&mutex1,NULL);
    pthread_mutex_init(&mutex2,NULL);
    pthread_cond_init(&wait_empty_buffer1, NULL);
    pthread_cond_init(&wait_empty_buffer2, NULL);
    pthread_cond_init(&wait_full_buffer1, NULL);
    pthread_cond_init(&wait_full_buffer2, NULL);

    pthread_create(&tid1,NULL,consume,NULL);
    pthread_create(&tid2,NULL,produce,NULL);
    pthread_create(&tid3,NULL,calculate,NULL);

    pthread_join(tid1,NULL);
    pthread_join(tid2,NULL);
    pthread_join(tid3,NULL);
    return 0;
}





