#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include "logging.h"
#include "types.h"

int logOP(action a, int i , int dur, int pl){
    
    char *actions[] = { "IWANT", "RECVD", "ENTER", "IAMIN", "TIMUP", "2LATE", "CLOSD", "FAILD", "GAVUP"};

    int pid = getpid();
    long int tid = pthread_self();

    char str[100];

    time_t seconds; 
    if( time(&seconds) == -1 )
        return ERROR; 

    if( sprintf(str, "%ld ; %d ; %d ; %ld ; %d ; %d ; %s\n", seconds, i, pid, tid, dur, pl, actions[a]) < 0 )
        return ERROR;

    if( write(STDOUT_FILENO, str, strlen(str)) != strlen(str) )
        return ERROR;

    return OK;
}