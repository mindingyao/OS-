#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<math.h>
#include<time.h>

float res = 0, res1 = 0, res2 = 0;
int array[50000];
int pre = 40000;

void master(int pre)
{
    int i;
    for (i = 1; i<=(pre+1)/2; i++)
    {
	res1 += 1.0/array[i];
    }
}
void *worker(void *arg)
{
    int *num, i;
    num = arg;
    for (i = (*num+1)/2+1; i<=*num; i++)
    {
	res2 += 1.0/array[i];
    }
}


int main()
{
    memset(array, 0, sizeof(array));
    int i;
    pthread_t tid;
    for (i=1; i<=pre; i++)
    {
	if (i%2 == 1)
	{
	    array[i] = 2*(i-1)+1;
	}
	else
	{
	    array[i] = -2*(i-1)-1;
	}
    }
    clock_t start, end;
    start = clock();
    pthread_create(&tid, NULL, worker, &pre);
    master(pre);
    pthread_join(tid, NULL);
    res = res1+res2;
    end = clock();
    printf("res1 = %f, res2 = %f, res = %f, PI = %f\n", res1, res2, res, res*4);
    printf("runtime: %.4f\n", (double)(start-end)/CLOCKS_PER_SEC);
    return 0;
}

