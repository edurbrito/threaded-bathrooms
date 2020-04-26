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

Shared_memory *shmem; 

pthread_t threads[MAX_THREADS];
pthread_t closingThreads[MAX_THREADS]; 

int threadsAvailable = MAX_THREADS;

int thread_pos = 0;
int closing_pos = 0;

pthread_mutex_t threads_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t threads_cond = PTHREAD_COND_INITIALIZER;

pthread_mutex_t pos_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t closing_lock = PTHREAD_MUTEX_INITIALIZER;

void * handle_request(void *arg){

    data threadData = *(data *) arg;
    free(arg);

    char fifoName[FIFONAME_SIZE + 50];
    sprintf(fifoName,"/tmp/%d.%lu",threadData.msg.pid,threadData.msg.tid);

    int fd;
    if((fd = open(fifoName,O_WRONLY)) == -1){ // Opening Client FIFO
        fprintf(stderr,"Error opening '%s' in WRITEONLY mode.\n",fifoName);
        //logOP(GAVUP,msg->i,msg->dur,msg->pl);
        return NULL;
    }

    threadData.msg.pid = getppid();
    threadData.msg.tid = pthread_self();

    if(write(fd, &threadData.msg, sizeof(message)) == -1){ // Writing to Client FIFO
        fprintf(stderr,"Error writing response to Client.\n");
        //logOP(GAVUP,msg->i,msg->dur,msg->pl);
        return NULL;
    }
    //logOP(ENTER,msg->i,msg->dur,msg->pl);

    close(fd);

    unsigned int remainingTime = threadData.msg.dur;
    usleep(remainingTime); // Just consuming the time

    pthread_mutex_lock(&pos_lock);
    thread_pos = threadData.myThreadPos;
    pthread_mutex_unlock(&pos_lock);

    pthread_mutex_lock(&threads_lock);
    threadsAvailable++;
    pthread_cond_signal(&threads_cond);
    pthread_mutex_unlock(&threads_lock);

    return NULL;
}

void * refuse_request(void *arg){

    data threadData = *(data *) arg;
    free(arg);
    
    char fifoName[FIFONAME_SIZE + 50];
    sprintf(fifoName,"/tmp/%d.%lu",threadData.msg.pid,threadData.msg.tid);
    
    int fd;
    if((fd = open(fifoName,O_WRONLY)) == -1){
        fprintf(stderr,"Error opening '%s' in WRITEONLY mode.\n",fifoName);
        //logOP(GAVUP,msg.i,msg.dur,msg.pl);
        return NULL;
    }

    threadData.msg.pid = getppid();
    threadData.msg.tid = pthread_self();
    threadData.msg.pl = -1;
    threadData.msg.dur = -1;

    if(write(fd, &threadData.msg, sizeof(message)) == -1){
        fprintf(stderr,"Error writing response to Client.\n");
        //logOP(GAVUP,msg.i,msg.dur,msg.pl);
        return NULL;
    }
    //logOP(TLATE,msg.i,msg.dur,msg.pl);

    close(fd);

    pthread_mutex_lock(&closing_lock);
    closing_pos = threadData.myThreadPos;
    pthread_mutex_unlock(&closing_lock);

    pthread_mutex_lock(&threads_lock);
    threadsAvailable++;
    pthread_cond_signal(&threads_cond);
    pthread_mutex_unlock(&threads_lock);

    return NULL;
}

void * server_closing(void * arg){
    
    int noPlaceId = 0;

    while(shmem->server_open == 0){
        pthread_mutex_lock(&threads_lock);
            while(threadsAvailable == 0){
                pthread_cond_wait(&threads_cond, &threads_lock);
            }
        pthread_mutex_unlock(&threads_lock);

        data * threadData = (data *) malloc(sizeof(data));
        
        // Read message from client if it exists (without blocking)
        int r = read(server_fd, &threadData->msg, sizeof(message)); 

        // No message to read yet or error
        if(r <= 0){
            // Error that does not concern the NON BLOCKING MODE -> EXIT
            if(r < 0 && isNotNonBlockingError() == OK){
                free(threadData);
                break;
            }
             // Nothing to read
            else{
                // logOP(GAVUP,msg->i,msg->dur,msg->pl);
                free(threadData);
                continue;
            }
        }

        noPlaceId ++;

        //logOP(RECVD,msg->i,msg->dur,msg->pl);

        printMsg(&threadData->msg);

        pthread_mutex_lock(&threads_lock);
        threadsAvailable--;
        pthread_mutex_unlock(&threads_lock);
        
        threadData->myThreadPos = closing_pos;
        // Creates new thread in respective position
        if(pthread_create(&closingThreads[closing_pos], NULL, refuse_request, threadData) != OK){
            free(threadData);
            fprintf(stderr,"Error creating thread.\n");
            break;
        }

        pthread_mutex_lock(&closing_lock);
        closing_pos++;
        pthread_mutex_unlock(&closing_lock);     
    }

    // Wait for the threads that will inform clients that made requests when server was closing
    for(int i = 0; i < noPlaceId && i < MAX_THREADS; i++){
        pthread_join(closingThreads[i],NULL);
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

    pthread_t timechecker;

    if ((shmem = create_shared_memory(SHM_NAME, sizeof(int))) == NULL){
        perror("SERVER: could not attach shared memory");
        exit(ERROR);
    }
    init_sync_objects_in_shared_memory(shmem); 

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

    // Don't block FIFO if no requests are ready to read
    setNonBlockingFifo(server_fd);

    int terminated[] = {a.nsecs, 0}; // Second element will be 1 if time is over, creating no more threads on main thread
    
    // Creating the synchronous thread that just checks if the time ended
    // Notifying the main thread with the @p terminated variable
    pthread_create(&timechecker, NULL, timeChecker, (void *) terminated);
    pthread_detach(timechecker);

    int placeId = 0;

    // Receive client's requests during defined time interval
    while(!terminated[1]){
        
        pthread_mutex_lock(&threads_lock);
        while(threadsAvailable == 0){
            pthread_cond_wait(&threads_cond, &threads_lock);
        }
        pthread_mutex_unlock(&threads_lock);

        data * threadData = (data*)malloc(sizeof(data));
        
        // Read message from client if it exists (without blocking)
        int r = read(server_fd, &threadData->msg, sizeof(message)); 

        // No message to read yet or error
        if(r <= 0){
            // Error that does not concern the NON BLOCKING MODE -> EXIT
            if(r < 0 && isNotNonBlockingError() == OK){
                free(threadData);
                break;
            }
             // Nothing to read
            else{
                // logOP(GAVUP,msg->i,msg->dur,msg->pl);
                free(threadData);
                continue;
            }
        }

        //logOP(RECVD,msg->i,msg->dur,msg->pl);

        printMsg(&threadData->msg);
        
        // Create thread to handle the request of the client

        // Set client's place number
        threadData->msg.pl = placeId;
        placeId ++;

        pthread_mutex_lock(&threads_lock);
        threadsAvailable--;
        pthread_mutex_unlock(&threads_lock);
        
        threadData->myThreadPos = thread_pos;
        // Creates new thread in respective position
        if(pthread_create(&threads[thread_pos], NULL, handle_request, threadData) != OK){
            free(threadData);
            fprintf(stderr,"Error creating thread.\n");
            break;
        }

        pthread_mutex_lock(&pos_lock);
        thread_pos++;
        pthread_mutex_unlock(&pos_lock);      
        
    }

    //logOP(TIMUP,place_in - 1, 0, place_in);
    
    printf("Time is over... Threads will be joined.\n");

    // Create thread to handle requests while server is closing
    pthread_t blockedServerThread;
    pthread_mutex_lock(&threads_lock);
        while(threadsAvailable == 0){
            pthread_cond_wait(&threads_cond, &threads_lock);
        }
        threadsAvailable--;
    pthread_mutex_unlock(&threads_lock);

    if(pthread_create(&blockedServerThread, NULL, server_closing,NULL) != OK){
        fprintf(stderr,"Error creating thread.\n");

        if(unlink(fifoName) < 0){
            fprintf(stderr, "Error when destroying '%s'.\n",fifoName);
            exit(ERROR);
        }
        close(server_fd);

        exit(ERROR);
    }

    // Wait for all threads to finish except the ones thrown when server was already closing

    for(int i = 0; i < placeId && i < MAX_THREADS; i++){
        pthread_join(threads[i],NULL);
        pthread_mutex_lock(&threads_lock);
        threadsAvailable++;
        pthread_cond_signal(&threads_cond);
        pthread_mutex_unlock(&threads_lock);
    }

    printf("Finished waiting for clients. \n");
    pthread_mutex_lock(&shmem->request_lock);
    shmem->server_open = 1; // Stop receiving messages
    pthread_mutex_unlock(&shmem->request_lock);

    // Wait for the thread that is handling the requests sent when the server was closing
    pthread_join(blockedServerThread,NULL);

    // Close the server and stop receiving requests
    if(unlink(fifoName) < 0){
        fprintf(stderr, "Error when destroying '%s'.\n",fifoName);
        exit(ERROR);
    }
    
    close(server_fd);

    destroy_shared_memory(shmem, sizeof(int)); 

    pthread_exit(OK);
}