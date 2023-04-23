#include "queue.h"

// Initialize queue
int initQueue(QUEUE **q) 
{
    *q = (QUEUE *) malloc(sizeof(QUEUE));
    if (*q == NULL)
    {
        perror("malloc");
        return EXIT_FAILURE;
    }
    (*q)->head = (*q)->tail = NULL;
    (*q)->size = 0;
    return EXIT_SUCCESS;
}

// Insert an item in the queue
int enqueue(int x, QUEUE *q) 
{
    if (q == NULL)
    {
        return EXIT_FAILURE;
    }
    
    NODE *temp = (NODE *) malloc(sizeof(NODE));
    if (temp == NULL) 
    {
        perror("malloc");
        return EXIT_FAILURE;
    }

    temp->data = x;
    temp->next = NULL;

    // if queue is empty
    if (q->size == 0) 
    { 
        q->head = q->tail = temp;
    }
    else 
    {
        q->tail->next = temp;
        q->tail = temp;
    }

    (q->size)++;
    return EXIT_SUCCESS;
}

// Removes the next element from the queue
int dequeue(QUEUE *q, int *retval) {
    if (q == NULL || q->size == 0)
    {
        return EXIT_FAILURE;
    }
    NODE *temp = q->head;
    q->head = q->head->next;
    if (q->size == 1) 
    {
        q->tail = NULL;
    }

    *retval = temp->data;
    free(temp);
    (q->size)--;
    return EXIT_SUCCESS;
}

// Checks if queue is empty
int isEmpty(QUEUE *q) {
    if (q->size == 0)
    {
        return 1;
    }
    return 0;
}

// Prints the Queue (for debugging purposes)
void printQueue(QUEUE *q) {
	if (q == NULL)	
	{
		return;
	}
	NODE *temp = (NODE *) malloc(sizeof(NODE));
	if (temp == NULL) 
	{
		perror("malloc");
		exit(-1);
	}
	temp = q->head;
	while(temp != NULL) 
	{
		printf("%d ", temp->data);
		temp = temp->next;
	}
	printf("\n");
}

/*
int main() {
    QUEUE *q;
    initQueue(&q);
	
    printf("%d\n", isEmpty(q));
    int i, N = 20;
    for(i = 1; i <= N; i++)
        enqueue(i, q);
        
    printf("%d\n", isEmpty(q));
    printQueue(q);
    int t;
    for(i = 0; i < N; i++)
    {
        dequeue(q, &t);
        printf("%d %d\n", t, isEmpty(q));
    }
    printf("%d\n", isEmpty(q));
    printQueue(q);
    free(q);
    return 0;
}
*/
