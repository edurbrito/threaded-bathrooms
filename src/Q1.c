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

    unsigned int remainingTime = msg.dur;
    while((remainingTime = usleep(remainingTime)) != 0){}

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
    pthread_t threads[MAX_CLOSING_SERVER_THREADS];
    int threadNum = 0;
    free(arg);

    while(1){

        message * msg = (message *) malloc(sizeof(message));
        
        int r;
        if((r = read(fd,msg, sizeof(message))) < 0){
            if(isNonBlockingError() == OK){
                return NULL;
            }
            else
                continue;
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
    int placeNum = 0, threadNum = 0;
    pthread_t * threads = (pthread_t *)malloc(0);

    if (threads == NULL) {
        printf("Error! memory not allocated.");
        exit(ERROR);
    }

    time_t t = time(NULL) + a.nsecs;

    if((fd = open(fifoName,O_RDONLY|O_NONBLOCK)) == -1){
        fprintf(stderr,"Error opening %s in READONLY mode.\n",fifoName);
        if(unlink(fifoName) < 0){
            fprintf(stderr, "Error when destroying %s.'\n",fifoName);
            exit(ERROR);
        }
        exit(ERROR);
    }

    setNonBlockingFifo(fd);

    while(time(NULL) < t){

        message * msg = (message *) malloc(sizeof(message));
        
        int r;
        // Read message from client
        if((r = read(fd,msg, sizeof(message))) < 0){
            if(isNonBlockingError() == OK){
                free(msg);
                break;
            }
            else
                continue;
        }
        else if(r == 0){
            free(msg);
            continue;
        }

        printMsg(msg);
        
        // Set client's place number
        msg->pl = placeNum;
        placeNum ++;

        threads = (pthread_t *) realloc(threads, (threadNum + 1)*sizeof(pthread_t));
        if (threads == NULL) {
            fprintf(stderr,"Reallocation failed\n");
            exit(ERROR);
        }

        // Create thread to handle the request of the client
        pthread_create(&threads[threadNum], NULL, handle_request, msg);
        threadNum ++;        
    }

    // Deal with threads when the server is closing
    int * fd1 = (int*)malloc(sizeof(int));
    *fd1 = fd;
    threads = (pthread_t *) realloc(threads, (threadNum + 1)*sizeof(pthread_t));
    pthread_create(&threads[threadNum], NULL, server_closing,fd1);

    // Wait for all threads to finish except the ones thrown when server was already closing
    for(int i = 0; i < threadNum ; i++){
        pthread_join(threads[i],NULL);
    }

    if(unlink(fifoName) < 0){
        fprintf(stderr, "Error when destroying %s.'\n",fifoName);
        exit(ERROR);
    }

    free(fd1);
    free(threads);
    
    printf("Fifo %s destroyed.\n",fifoName);
    exit(ERROR);

    /*if ( logOP(CLOSD,2,24,4) != OK )
        return ERROR;*/
    
    close(fd);

    exit(0);

}