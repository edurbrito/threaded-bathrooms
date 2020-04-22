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

int server_fd;

void * handle_request(void *arg){

    message msg = *(message *) arg;
    char fifoName[FIFONAME_SIZE];
    sprintf(fifoName,"/tmp/%d.%lu",msg.pid,msg.tid);
    int fd;

    if((fd = open(fifoName,O_WRONLY)) == -1){
        fprintf(stderr,"Error opening %s in WRITEONLY mode.\n",fifoName);
        return NULL;
    }

    msg.pid = getppid();
    msg.tid = pthread_self();

    if(write(fd, &msg, sizeof(message)) == -1){
        fprintf(stderr,"Error writing response to Client.\n");
        return NULL;
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
        return NULL;
    }

    msg.pid = getppid();
    msg.tid = pthread_self();
    msg.pl = -1;
    msg.dur = -1;

    if(write(fd, &msg, sizeof(message)) == -1){
        fprintf(stderr,"Error writing response to Client.\n");
        return NULL;
    }
    close(fd);

    free(arg);

    return NULL;
}

void * server_closing(void * arg){

    int threadNum = *(int *)arg;
    pthread_t threads[MAX_THREADS-threadNum];
    free(arg);

    while(1){

        message * msg = (message *) malloc(sizeof(message));
        
        int r;
        if((r = read(server_fd,msg, sizeof(message))) < 0){
            if(isNonBlockingError() == OK){
                return NULL;
            }
            else
                continue;
        }
        else if(r == 0){
            free(msg);
            break;
        }

        printMsg(msg);

        pthread_create(&threads[threadNum], NULL, refuse_request, msg);
        threadNum ++;
        
    }

    // Wait for the threads that will inform clients that made requests when server was closing
    for(int i = 0; i < threadNum ; i++){
        pthread_join(threads[i],NULL);
    }

    return NULL;
}

int main(int argc, char * argv[]){

    args a;

    // Check arguments
    if (checkArgs(argc, argv, &a, Q) != OK ){
        fprintf(stderr,"Usage: %s <-t nsecs> [-l nplaces] [-n nthreads] fifoname\n",argv[0]);
        logOP(CLOSD,1,12,2);
        exit(ERROR);
    }

    printArgs(&a);
    
    
    char fifoName[FIFONAME_SIZE+5];
    sprintf(fifoName,"/tmp/%s",a.fifoName);

    // Create Fifo to receive client's requests
    if(mkfifo(fifoName,0660) < 0){
        if (errno == EEXIST){
            printf("Fifo '%s' already exists.\n",fifoName);
        }
        else {
            fprintf(stderr, "Could not create Fifo %s.\n",fifoName); 
            exit(ERROR);
        }
    }

    int placeNum = 0, threadNum = 0;
    pthread_t threads[MAX_THREADS];

    time_t t = time(NULL) + a.nsecs;
    
    // Open Fifo in READ ONLY mode to read client's requests
    if((server_fd = open(fifoName,O_RDONLY|O_NONBLOCK)) == -1){
        fprintf(stderr,"Error opening %s in READONLY mode.\n",fifoName);
        if(unlink(fifoName) < 0){
            fprintf(stderr, "Error when destroying %s.'\n",fifoName);
            exit(ERROR);
        }
        exit(ERROR);
    }

    // Don't block Fifo if no requests are ready to read
    setNonBlockingFifo(server_fd);

    // Receive client's requests during defined time interval
    while(time(NULL) < t){

        message * msg = (message *) malloc(sizeof(message));
        
        int r;
        // Read message from client if it exists (without blocking)
        if((r = read(server_fd,msg, sizeof(message))) < 0){
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
        
        // Create thread to handle the request of the client
        if(pthread_create(&threads[threadNum], NULL, handle_request, msg) != OK){
            free(msg);
            fprintf(stderr,"Error creating thread.\n");
            break;
        }

        threadNum ++;        
    }

    int * threadNumArg =  (int *) malloc(sizeof(t)); 

    // Create thread to handle requests while server is closing
    pthread_create(&threads[threadNum], NULL, server_closing,threadNumArg);
    threadNum ++;  

    // Wait for all threads to finish except the ones thrown when server was already closing
    for(int i = 0; i < threadNum-1 ; i++){
        pthread_join(threads[i],NULL);
    }

    // Close the server -> stop receiving requests
    if(unlink(fifoName) < 0){
        fprintf(stderr, "Error when destroying %s.'\n",fifoName);
        exit(ERROR);
    }

    printf("Fifo %s destroyed.\n",fifoName);

    /*if ( logOP(CLOSD,2,24,4) != OK )
        return ERROR;*/
    
    // Wait for the thread that is handling the requests sent when the server was closing
    pthread_join(threads[threadNum-1],NULL);

    close(server_fd);

    pthread_exit(OK);

}