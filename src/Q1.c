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

void * handle_request(void *arg){

    message msg = *(message *) arg;
    char fifoName[FIFONAME_SIZE];
    sprintf(fifoName,"/tmp/%d.%lu",msg.pid,msg.tid);
    int fd;

    if((fd = open(fifoName,O_WRONLY)) == -1){
        fprintf(stderr,"Error opening %s in WRITEONLY mode.\n",fifoName);
        //exit(ERROR);
    }

    msg.pid = getppid();
    msg.tid = pthread_self();

    if(write(fd, &msg, sizeof(message)) == -1){
        fprintf(stderr,"Error writing response to Client.\n");
        //exit(ERROR);
    }
    close(fd);

    free(arg);

    unsigned int remainingSec = msg.dur;
    while((remainingSec = sleep(remainingSec)) != 0){}

    return NULL;
}

void * refuse_request(void *arg){

    message msg = *(message *) arg;
    char fifoName[FIFONAME_SIZE];
    sprintf(fifoName,"/tmp/%d.%lu",msg.pid,msg.tid);
    int fd;

    if((fd = open(fifoName,O_WRONLY)) == -1){
        fprintf(stderr,"Error opening %s in WRITEONLY mode.\n",fifoName);
        //exit(ERROR);
    }

    msg.pid = getppid();
    msg.tid = pthread_self();
    msg.pl = -1;
    msg.dur = -1;

    if(write(fd, &msg, sizeof(message)) == -1){
        fprintf(stderr,"Error writing response to Client.\n");
        //exit(ERROR);
    }
    close(fd);

    free(arg);

    return NULL;
}

void * server_closing(void * arg){

    int fd = *(int*)arg;
    pthread_t threads[MAX_THREADS];
    int threadNum = 0;
    free(arg);

    while(1){

        message * msg = (message *) malloc(sizeof(message));
        
        int r;
        if((r = read(fd,msg, sizeof(message))) < 0){
            free(msg);
            fprintf(stderr, "Couldn't read from %d.",fd);
            break;  
        }
        else if(r == 0){
            free(msg);
            continue;
        }

        printMsg(msg);

        pthread_create(&threads[threadNum], NULL, refuse_request, msg);
        threadNum ++;
        
    };

    return NULL;
}

int main(int argc, char * argv[]){

    args a;

    if (checkArgs(argc, argv, &a, Q) != OK ){
        fprintf(stderr,"Usage: %s <-t nsecs> [-l nplaces] [-n nthreads] fifoname\n",argv[0]);
        logOP(CLOSD,1,12,2);
        exit(ERROR);
    }

    printArgs(&a);
    
    char fifoName[FIFONAME_SIZE+5];
    sprintf(fifoName,"/tmp/%s",a.fifoName);

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
    time_t t = time(NULL) + a.nsecs;


    if((fd = open(fifoName,O_RDONLY)) == -1){
        printf("HERE\n");
        fprintf(stderr,"Error opening %s in READONLY mode.\n",fifoName);
        if(unlink(fifoName) < 0){
            fprintf(stderr, "Error when destroying %s.'\n",fifoName);
            exit(ERROR);
        }
    }

    while(time(NULL) < t){

        message * msg = (message *) malloc(sizeof(message));
        
        int r;
        if((r = read(fd,msg, sizeof(message))) < 0){
            free(msg);
            fprintf(stderr, "Couldn't read from %d.",fd);
            break;  
        }
        else if(r == 0){
            free(msg);
            continue;
        }

        printMsg(msg);

        msg->pl = placeNum;
        placeNum ++;

        pthread_create(&threads[threadNum], NULL, handle_request, msg);
        threadNum ++;        
    }

    int * fd1 = (int*)malloc(sizeof(int));
    *fd1 = fd;
    pthread_create(&threads[threadNum], NULL, server_closing,fd1);

    for(int i = 0; i < threadNum ; i++){
        pthread_join(threads[i],NULL);
    }

    if(unlink(fifoName) < 0){
        fprintf(stderr, "Error when destroying %s.'\n",fifoName);
        exit(ERROR);
    }
    
    printf("Fifo %s destroyed.\n",fifoName);
    exit(ERROR);

    /*if ( logOP(CLOSD,2,24,4) != OK )
        return ERROR;*/
    
    close(fd);

    exit(0);

}