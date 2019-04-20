#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<math.h>

#define pre 40000

float res = 0;
int array[50000];

struct param
{
    int start;
    int end;
};

void *compute(void *arg)
{
    int i;
    struct param *p;
    p = (struct param *)arg;
    for (i =  p->start; i<=p->end; i++)
    {
	res += 1.0/array[i];
    }
}

int main()
{
    int i, n, epoch;
    pthread_t tid[20];

    printf("进程数为:");
    scanf("%d", &n);
    epoch = pre/n;
    for (i=1; i<=pre; i++)
    {
	if (i%2==1)
	    array[i] = 2*(i-1)+1;
	else
	    array[i] = -2*(i-1)-1;
    }

    for (i=1; i<=n;i++)
    {
	struct param *p;
	p = (struct param *)malloc(sizeof(struct param));
	if (i!=n)
	{
	    p->start = 1 + (i-1)*epoch;
	    p->end = i*epoch;
	}
	else
	{
	    p->start = 1 + (i-1)*epoch;
	    p->end = pre;
	}
	pthread_create(&tid[i], NULL, compute, p);
    }
    for (i=1; i<=n; i++)
    {
	pthread_join(tid[i], NULL);
    }
    printf("res = %f, PI = %f\n", res, res*4);
    return 0;
}



