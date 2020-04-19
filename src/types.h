#ifndef TYPES_H
#define TYPES_H

#include <pthread.h>

#define OK 0
#define ERROR 1
#define FIFONAME_SIZE 200
#define MAX_THREADS 20

typedef enum{ U, Q } caller;

typedef struct args {
    int nsecs;
    /*int nplaces;
    int nthreads;*/
    char fifoname[FIFONAME_SIZE];
} args;

typedef struct message {
    int i;
    pid_t pid;
    pthread_t tid;
    int dur;
    int pl;
    char fifoname[FIFONAME_SIZE];
} message;

typedef struct clientparameters {
    char fifoname[50];
    int id;
}clientparameters;


#endif