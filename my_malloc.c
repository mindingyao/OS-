#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define POINTERS 1000
#define REGION_SIZE (4000 + 1000*16)
 
/* memory control block */
typedef struct block {
    struct block *next;   
    int in_use;                    
    int size;                     
}BLOCK;
 
char region[REGION_SIZE];
static BLOCK *free_list;                  
 
void my_malloc_init()
{
    free_list = (BLOCK *)region;
    free_list->next = NULL;
    free_list->in_use = 0;
    free_list->size = REGION_SIZE-sizeof(BLOCK);
}
 
 
void *my_malloc(int num_bytes)
{
    BLOCK *ptr = free_list;
    BLOCK *free_block;
    /* find a free and big-enough chunk */
    while(ptr)
    {
        if(ptr->in_use == 0 && ptr->size >= num_bytes)
            break;
        ptr = ptr->next;
    }
 
 
    /* create a new free mem control block */
    free_block = (BLOCK *) ((char *)(ptr + 1) + num_bytes);
    free_block->next = NULL;
    free_block->in_use = 0;
    free_block->size  = ptr->size - num_bytes - sizeof(BLOCK);
    free_block->next  = ptr->next;
 
    ptr->in_use = 1;
    ptr->size = num_bytes;
    ptr->next = free_block;
 
    return (void *)(ptr + 1);
}
 
 
void my_free(void *ptr)
{
    BLOCK *pre_block = free_list;
    BLOCK *this_block = (BLOCK *)ptr -1;
    BLOCK *next_block = this_block->next;
    if(this_block->in_use == 1)
    {
        this_block->in_use = 0;
    }
    /* merge right block*/
    if(next_block && next_block->in_use == 0)
    {
        this_block->next = next_block->next;
        this_block->size = this_block->size + sizeof(BLOCK) + next_block->size;
    }
    /* merge left block */
    while(pre_block && pre_block->next != this_block)
    {
        pre_block = pre_block->next;
    }
 
    if(pre_block && pre_block->in_use == 0)
    {
        pre_block->next = this_block->next;
        pre_block->size = pre_block->size + sizeof(BLOCK) + this_block->size;
    }
}
 



 
 

void print_free_memory()
{
    BLOCK *ptr = free_list;
    int index = 0;
    printf("\n------ memory space -------\n");
    while(ptr)
    {
        printf("-------- BLOCK[%d] ---------\n", index++);
        printf("[address]:%p\n", ptr);
        printf("[size(bytes)]:%d\n", ptr->size);
        printf("[in_use]:%s\n", (ptr->in_use? "USED" : "FREE"));
        ptr = ptr->next;
    }
}


void test0()
{
    int size;
    void *p1, *p2;

    puts("Test0");

    p1 = my_malloc(10);
    print_free_memory();

    p2 = my_malloc(20);
    print_free_memory();

    my_free(p1);
    print_free_memory();

    my_free(p2);
    print_free_memory();
}

void test1()
{
    void *array[POINTERS];
    int i;
    void *p;

    puts("Test1");
    for (i = 0; i < POINTERS; i++) {
        p = my_malloc(4);
        array[i] = p;
    }

    for (i = 0; i < POINTERS; i++) {
        p = array[i];
        my_free(p);
    }

    print_free_memory();
}

void test2()
{
    void *array[POINTERS];
    int i;
    void *p;

    puts("Test2");
    for (i = 0; i < POINTERS; i++) {
        p = my_malloc(4);
        array[i] = p;
    }

    for (i = POINTERS - 1; i >= 0; i--) {
        p = array[i];
        my_free(p);
    }

    print_free_memory();
}

void test3()
{
    void *array[POINTERS];
    int i;
    void *p;

    puts("Test3");
    for (i = 0; i < POINTERS; i++) {
        p = my_malloc(4);
        array[i] = p;
    }

    for (i = 0; i < POINTERS; i += 2) {
        p = array[i];
        my_free(p);
    }

    for (i = 1; i < POINTERS; i += 2) {
        p = array[i];
        my_free(p);
    }

    print_free_memory();
}

int main()
{
    my_malloc_init();
    test0();
    test1();
    test2();
    test3();
    puts("Finished");
    return 0;
}
