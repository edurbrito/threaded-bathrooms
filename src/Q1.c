#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/file.h>
#include <signal.h> 
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include "utils.h"
#include "types.h"
#include "logging.h"

void *thr_func(void *arg)
{
    struct message msg = *(message *) arg;
    char fifoName[FIFONAME_SIZE];
    sprintf(fifoName,"/tmp/%d.%lu",msg.pid,msg.tid);

    int fd = open(fifoName,O_WRONLY);
    if(fd == -1){
        fprintf(stderr,"Erro opening %s in WRITEONLY mode.\n",fifoName);
        exit(ERROR);
    }

    msg.pid = getppid();
    msg.tid = pthread_self();

    write(fd, &msg, sizeof(message));

    free(arg);

    pthread_exit(0);
}

int main(int argc, char * argv[]){

    args a;

    if (checkArgs(argc, argv, &a, Q) != OK ){
        fprintf(stderr,"Usage: %s <-t nsecs> [-l nplaces] [-n nthreads] fifoname\n",argv[0]);
        logOP(CLOSD,1,12,2);
        exit(ERROR);
    }

    printArgs(&a);

    time_t endwait;
    time_t start = time(NULL);
    time_t seconds = a.nsecs; // end loop after this time has elapsed
    endwait = start + seconds;
    
    char fifoName[FIFONAME_SIZE+5];
    sprintf(fifoName,"/tmp/%s",a.fifoname);

    if(mkfifo(fifoName,0660) < 0){
        if (errno == EEXIST){
            printf("Fifo '%s' already exists.\n",fifoName);
        }
        else {
            fprintf(stderr, "Could not create Fifo %s.\n",fifoName); 
            exit(ERROR);
        }
    }

    int fd;
    int placeNum = 0;
    int threadNum = 0;
    pthread_t threads[MAX_THREADS];

    while(start < endwait){

        if ((fd = open(fifoName,O_RDONLY|O_NONBLOCK)) == OK){

            struct message * msg = (struct message *) malloc(sizeof(struct message));
            
            int r = read(fd,msg, sizeof(struct message));
            if((r = read(fd,msg, sizeof(struct message))) != OK){
                fprintf(stderr, "Couldn't read from %d.",fd);
                exit(ERROR);
            }
            else if(r == sizeof(struct message)){
                msg->pl = placeNum;
                placeNum ++;

                pthread_create(&threads[threadNum], NULL, thr_func, (void *)msg);
                threadNum ++;
            }
        }
        start = time(NULL);
    }

    for(int i = 0; i < threadNum; i++){
        pthread_join(threads[i],NULL);
    }

    close(fd);

    if(unlink(fifoName) < 0){
        fprintf(stderr, "Error when destroying %s.'\n",fifoName);
        exit(ERROR);
    }
    
    printf("Fifo %s destroyed.\n",fifoName);
    exit(ERROR);

    /*if ( logOP(CLOSD,2,24,4) != OK )
        return ERROR;*/

    pthread_exit(OK);

}