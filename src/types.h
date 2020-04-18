#ifndef TYPES_H
#define TYPES_H

#define OK 0
#define ERROR 1

typedef enum{ U, Q } caller;

typedef struct args {
    int nsecs;
    int nplaces;
    int nthreads;
    char fifoname[200];
} args;

#endif