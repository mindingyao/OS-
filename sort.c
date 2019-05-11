#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<time.h>

#define size 20

int array[size];

void *worker(void *arg)
{
    int i, j;
    int tmp;
    for (i=0; i<size/2; i++)
    {
	for (j=i+1; j<size/2; j++)
	{
	    if (array[i]>array[j])
	    {
		tmp = array[i];
		array[i] = array[j];
		array[j] = tmp;
	    }
	}
    }
}

void master()
{
    
    int i, j;
    int tmp;
    for (i=size/2; i<size; i++)
    {
	for (j=i+1; j<size; j++)
	{
	    if (array[i]>array[j])
	    {
		tmp = array[i];
		array[i] = array[j];
		array[j] = tmp;
	    }
	}
    }
}

void merge()
{
    int tmp[size];
    int i=0, j=size/2,t=0,k;
    while(i<size/2 && j<size)
    {
	if(array[i]<array[j])
	    tmp[t++] = array[i++];
	else
	    tmp[t++] = array[j++];
    }
    if (i==size/2)
	for(k=j; k<size; k++)
	    tmp[t++] = array[j++];
    if (j==size)
	for(k=i; k<size/2; k++)
	    tmp[t++] = array[i++];
    for (i=0; i<size; i++)
    {
	array[i] = tmp[i];
    }
}

int main()
{
    int i;
    pthread_t tid;
    srand((unsigned)time(0));
    for (i=0; i<size; i++)
	array[i] = rand()%100;
    printf("原始序列为：");
    for (i=0; i<size; i++)
    {	
	printf("%d ", array[i]);
    }
    printf("\n");
    pthread_create(&tid, NULL, worker, NULL);
    master();
    pthread_join(tid, NULL);
    printf("处理后的序列:");
    for (i=0; i<size; i++)
	printf("%d ", array[i]);
    printf("\n");
    merge();
    printf("排序好的序列:");
    for(i=0; i<size; i++)
    {
	printf("%d ", array[i]);
    }
    printf("\n");
    return 0;
}
