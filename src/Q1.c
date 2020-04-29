#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h> 
#include "utils.h"
#include "types.h"

int server_fd;

int threadsAvailable = MAX_THREADS;

// Used to wait for available threads without busy waiting
pthread_mutex_t threads_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t threads_cond = PTHREAD_COND_INITIALIZER;

void * handle_request(void *arg){

    message * msg = (message *) arg;
    char fifoName[FIFONAME_SIZE + 50];
    sprintf(fifoName,"/tmp/%d.%lu",msg->pid,msg->tid);

    int fd;
    if((fd = open(fifoName,O_WRONLY)) == -1){ // Opening Client FIFO
        fprintf(stderr,"Error opening '%s' in WRITEONLY mode.\n",fifoName);
        logOP(GAVUP,msg->i,msg->dur,msg->pl);
        return NULL;
    }

    msg->pid = getppid();
    msg->tid = pthread_self();

    if(write(fd, msg, sizeof(message)) == -1){ // Writing to Client FIFO
        fprintf(stderr,"Error writing response to Client.\n");
        logOP(GAVUP,msg->i,msg->dur,msg->pl);
        return NULL;
    }
    logOP(ENTER,msg->i,msg->dur,msg->pl);

    close(fd);

    unsigned int remainingTime = msg->dur;
    usleep(remainingTime); // Just consuming the time

    logOP(TIMUP,msg->i, msg->dur, msg->pl);

    pthread_mutex_lock(&threads_lock);
    threadsAvailable++;
    pthread_cond_signal(&threads_cond);
    pthread_mutex_unlock(&threads_lock);

    free(arg);

    return NULL;
}

void * refuse_request(void *arg){

    message msg = *(message *) arg;
    
    char fifoName[FIFONAME_SIZE + 50];
    sprintf(fifoName,"/tmp/%d.%lu",msg.pid,msg.tid);
    
    int fd;
    if((fd = open(fifoName,O_WRONLY)) == -1){
        fprintf(stderr,"Error opening '%s' in WRITEONLY mode.\n",fifoName);
        free(arg);
        logOP(GAVUP,msg.i,msg.dur,msg.pl);
        return NULL;
    }

    msg.pid = getppid();
    msg.tid = pthread_self();
    msg.pl = -1;
    msg.dur = -1;

    if(write(fd, &msg, sizeof(message)) == -1){
        fprintf(stderr,"Error writing response to Client.\n");
        free(arg);
        logOP(GAVUP,msg.i,msg.dur,msg.pl);
        return NULL;
    }

    logOP(TLATE,msg.i,msg.dur,msg.pl);

    close(fd);

    free(arg);

    pthread_mutex_lock(&threads_lock);
    threadsAvailable++;
    pthread_cond_signal(&threads_cond);
    pthread_mutex_unlock(&threads_lock);

    return NULL;
}

void * server_closing(void * arg){

    pthread_t threads[MAX_THREADS]; 

    int threadNum = 0; // Restart Counting

    while(1){

        message * msg = (message *) malloc(sizeof(message));
        
        int r;
        if((r = read(server_fd,msg, sizeof(message))) < 0){
            if(isNotNonBlockingError() == OK){
                // logOP(GAVUP,msg->i,msg->dur,msg->pl);
                free(msg);
                break;
            }
            else{
                // logOP(GAVUP,msg->i,msg->dur,msg->pl);
                free(msg);
                continue;
            }
        }
        else if(r == 0){
            free(msg);
            break;
        }
        logOP(RECVD,msg->i,msg->dur,msg->pl);

        pthread_mutex_lock(&threads_lock);

            while(threadsAvailable <= 1){
                pthread_cond_wait(&threads_cond, &threads_lock);
            }

            threadsAvailable--;
            
        pthread_mutex_unlock(&threads_lock);

        // printMsg(msg);

        pthread_create(&threads[threadNum], NULL, refuse_request, msg);
        threadNum++;
    }

    // Wait for the threads that will inform clients that made requests when server was closing
    for(int i = 0; i < threadNum; i++){
        pthread_join(threads[i],NULL);
    }

    return NULL;
}

int main(int argc, char * argv[]){

    args a;

    if (checkArgs(argc, argv, &a, Q) != OK ){
        fprintf(stderr,"Usage: %s <-t nsecs> [-l nplaces] [-n nthreads] fifoname\n",argv[0]);
        exit(ERROR);
    }
       
    char fifoName[FIFONAME_SIZE + 50];
    sprintf(fifoName,"/tmp/%s",a.fifoName);

    int placeNum = 0, threadNum = 0;
    pthread_t threads[MAX_THREADS], timechecker;

    // Create FIFO to receive client's requests
    if(mkfifo(fifoName,0660) < 0){
        if (errno == EEXIST){
            printf("FIFO '%s' already exists.\n",fifoName);
        }
        else {
            fprintf(stderr, "Could not create FIFO '%s'.\n",fifoName); 
            exit(ERROR);
        }
    }

    // Open FIFO in READ ONLY mode to read client's requests
    if((server_fd = open(fifoName,O_RDONLY|O_NONBLOCK)) == -1){
        fprintf(stderr,"Error opening '%s' in READONLY mode.\n",fifoName);
        if(unlink(fifoName) < 0){
            fprintf(stderr, "Error when destroying '%s'.\n",fifoName);
            exit(ERROR);
        }
        exit(ERROR);
    }


    int terminated[] = {a.nsecs, 0}; // Second element will be 1 if time is over, creating no more threads on main thread
    
    // Creating the synchronous thread that just checks if the time ended
    // Notifying the main thread with the @p terminated variable
    pthread_create(&timechecker, NULL, timeChecker, (void *) terminated);
    pthread_detach(timechecker);

    // Receive client's requests during defined time interval
    while(!terminated[1]){

        message * msg = (message *) malloc(sizeof(message));
        
        int r;
        // Read message from client if it exists (without blocking)
        if((r = read(server_fd, msg, sizeof(message))) < 0){
            if(isNotNonBlockingError() == OK){
                // logOP(GAVUP,msg->i,msg->dur,msg->pl);
                free(msg);
                break;
            }
            else{
                // logOP(GAVUP,msg->i,msg->dur,msg->pl);
                free(msg);
                continue;
            }
        }
        else if(r == 0){ // EOF
            free(msg);
            continue;
        }        
        logOP(RECVD,msg->i,msg->dur,msg->pl);


        pthread_mutex_lock(&threads_lock);

            while(threadsAvailable <= 1){
                pthread_cond_wait(&threads_cond, &threads_lock);
            }

            threadsAvailable--;
            
        pthread_mutex_unlock(&threads_lock);

        // Set client's place number
        msg->pl = placeNum;
        placeNum++;

        // printMsg(msg);
        
        // Create thread to handle the request of the client
        if(pthread_create(&threads[threadNum], NULL, handle_request, msg) != OK){
            free(msg);
            fprintf(stderr,"Error creating thread.\n");
            break;
        }

        threadNum++;        
    }
    
    // printf("Time is over... Threads will be joined.\n");

    // Create thread to handle requests while server is closing
    pthread_create(&threads[threadNum], NULL, server_closing, NULL);

    // Wait for all threads to finish except the ones thrown when server was already closing
    for(int i = 0; i < threadNum; i++){
        pthread_join(threads[i],NULL);
    }
    
    // Wait for the thread that is handling the requests sent when the server was closing
    pthread_join(threads[threadNum],NULL);

    // Close the server and stop receiving requests
    if(unlink(fifoName) < 0){
        fprintf(stderr, "Error when destroying '%s'.\n",fifoName);
        exit(ERROR);
    }

    // printf("FIFO '%s' destroyed.\n",fifoName);
    
    close(server_fd);

    pthread_exit(OK);
}