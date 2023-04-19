#ifndef QUEUE_H
#define QUEUE_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct node 
{
   int data;
   struct node *next;
} NODE;

typedef struct 
{
   int size;
   struct node *head;
   struct node *tail;
} QUEUE;

int initQueue(QUEUE **q);

int enqueue(int x, QUEUE *q);

int dequeue(QUEUE *q, int *retval);

int isEmpty(QUEUE *q);

void printQueue(QUEUE *q);

#endif
