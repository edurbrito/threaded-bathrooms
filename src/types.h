#ifndef TYPES_H
#define TYPES_H

#define OK 0
#define ERROR 1
#define FIFONAME_SIZE 200
#define MAX_THREADS 1000

typedef enum{ U, Q } caller;

typedef struct args {
    int nsecs;
    /*int nplaces;
    int nthreads;*/
    char fifoName[FIFONAME_SIZE];
} args;

typedef struct message {
    int i;
    pid_t pid;
    pthread_t tid;
    int dur;
    int pl;
    char fifoName[FIFONAME_SIZE];
} message;

#endif