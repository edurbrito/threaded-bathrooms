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

int server_fd; // server file descriptor
int threadsAvailable = 50; // threads running at the same time / simultaneously
int placesAvailable = 50; // places available - will be updated with the user value if provided
int places[50] = {0}; 

// Used to wait for available threads without busy waiting
pthread_mutex_t threads_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t threads_cond = PTHREAD_COND_INITIALIZER;

// Used to wait for available places without busy waiting
pthread_mutex_t places_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t places_cond = PTHREAD_COND_INITIALIZER;

void incrementMutexes(){
    pthread_mutex_lock(&threads_lock);
    threadsAvailable++;
    pthread_cond_signal(&threads_cond);
    pthread_mutex_unlock(&threads_lock);

    pthread_mutex_lock(&places_lock);
    placesAvailable++;
    pthread_cond_signal(&places_cond);
    pthread_mutex_unlock(&places_lock);
}

void * handle_request(void *arg){

    message * msg = (message *) arg;
    char fifoName[FIFONAME_SIZE + 50];
    sprintf(fifoName,"/tmp/%d.%lu",msg->pid,msg->tid);

    int fd;
    if((fd = open(fifoName,O_WRONLY)) == -1){ // Opening Client FIFO
        fprintf(stderr,"Error opening '%s' in WRITEONLY mode.\n",fifoName);
        logOP(GAVUP,msg->i,msg->dur,msg->pl);
        incrementMutexes();
        free(arg);
        return NULL;
    }

    msg->pid = getppid();
    msg->tid = pthread_self();

    if(write(fd, msg, sizeof(message)) == -1){ // Writing to Client FIFO
        fprintf(stderr,"Error writing response to Client.\n");
        logOP(GAVUP,msg->i,msg->dur,msg->pl);
        incrementMutexes();
        free(arg);
        close(fd);
        return NULL;
    }
    logOP(ENTER,msg->i,msg->dur,msg->pl);

    close(fd);

    unsigned int remainingTime = msg->dur;
    usleep(1000 * remainingTime); // Just consuming the time

    logOP(TIMUP,msg->i, msg->dur, msg->pl);

    freePlace(places, msg->pl);
    incrementMutexes();

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
        logOP(GAVUP,msg.i,msg.dur,msg.pl);
        incrementMutexes();
        free(arg);
        return NULL;
    }

    msg.pid = getppid();
    msg.tid = pthread_self();
    msg.pl = -1;
    msg.dur = -1;

    if(write(fd, &msg, sizeof(message)) == -1){
        fprintf(stderr,"Error writing response to Client.\n");
        logOP(GAVUP,msg.i,msg.dur,msg.pl);
        incrementMutexes();
        free(arg);
        close(fd);
        return NULL;
    }

    logOP(TLATE,msg.i,msg.dur,msg.pl);

    close(fd);

    free(arg);

    incrementMutexes();

    return NULL;
}

void * server_closing(void * arg){

    int *server_opened = (int * )arg;

    pthread_t * threads = (pthread_t *) malloc(MAX_THREADS * sizeof(pthread_t));

    int threadNum = 0; // Start counting again

    while(1){

        message * msg = (message *) malloc(sizeof(message));
        
        int r;
        if((r = read(server_fd,msg, sizeof(message))) < 0){
            if(isNotNonBlockingError() == OK){
                free(msg);
                break;
            }
            else{
                free(msg);
                continue;
            }
        }
        else if(r == 0){
            // If server closed already and nothing is read then break
            if(!*server_opened){
                break;
            }
            free(msg);
            continue;
        }
        logOP(RECVD,msg->i,msg->dur,msg->pl);

        pthread_mutex_lock(&threads_lock);

            while(threadsAvailable <= 0){
                pthread_cond_wait(&threads_cond, &threads_lock);
            }
            threadsAvailable--;
            
        pthread_mutex_unlock(&threads_lock);

        pthread_create(&threads[threadNum], NULL, refuse_request, msg);

        threadNum++;

        if (threadNum % MAX_THREADS == 0){ // If reached the MAX limit of total threads
            // realloc more memory for threads
            int i = threadNum / MAX_THREADS + 1;
            threads = realloc(threads, i * MAX_THREADS * sizeof(pthread_t));
        } 
    }

    // Wait for the threads that will inform clients that made requests when server was closing
    for(int i = 0; i < threadNum; i++){
        pthread_join(threads[i],NULL);
    }   

    free(threads);

    return NULL;
}

int main(int argc, char * argv[]){

    args a;
    int requestCounter = 0;

    if (checkArgs(argc, argv, &a, Q) != OK ){
        fprintf(stderr,"Usage: %s <-t nsecs> <-l nplaces> <-n nthreads> fifoname\n",argv[0]);
        exit(ERROR);
    }

    if (ignoreSIGPIPE() != OK){
        fprintf(stderr,"Error ignoring SIGPIPE\n");
        exit(ERROR);
    }
       
    char fifoName[FIFONAME_SIZE + 50];
    sprintf(fifoName,"/tmp/%s",a.fifoName);

    if(a.nthreads > 0)
        threadsAvailable = a.nthreads;
    if(a.nplaces > 0){
        placesAvailable = a.nplaces;
    }
        

    int threadNum = 0;
    pthread_t timechecker; // timechecker will check the time left
    pthread_t * threads = (pthread_t *) malloc(MAX_THREADS * sizeof(pthread_t));
    

    // Create FIFO to receive client's requests
    if(mkfifo(fifoName,0660) < 0){
        if (errno == EEXIST){
            printf("FIFO '%s' already exists.\n",fifoName);
        }
        else {
            fprintf(stderr, "Could not create FIFO '%s'.\n",fifoName); 
            free(threads);
            exit(ERROR);
        }
    }

    // Open FIFO in READ ONLY mode to read client's requests
    if((server_fd = open(fifoName,O_RDONLY|O_NONBLOCK)) == -1){
        fprintf(stderr,"Error opening '%s' in READONLY mode.\n",fifoName);
        free(threads);
        if(unlink(fifoName) < 0){
            fprintf(stderr, "Error when destroying '%s'.\n",fifoName);
            exit(ERROR);
        }
        exit(ERROR);
    }


    int terminated[] = {a.nsecs, 0}; // Second element will be 1 if time is over, creating no more threads on main thread
    
    // Creating the synchronous thread that just checks if the time ended
    // Notifying the main thread with the @p terminated variable
    if(pthread_create(&timechecker, NULL, timeChecker, (void *) terminated) != OK){
        fprintf(stderr,"Error creating timeChecker thread.\n");
        free(threads);
        exit(ERROR);
    }
    
    if(pthread_detach(timechecker) != OK){
        fprintf(stderr,"Error detaching timeChecker thread.\n");
        free(threads);
        exit(ERROR);
    }

    // Receive client's requests during defined time interval
    while(!terminated[1]){

        message * msg = (message *) malloc(sizeof(message));
        
        int r;
        // Read message from client if it exists (without blocking)
        if((r = read(server_fd, msg, sizeof(message))) < 0){
            if(isNotNonBlockingError() == OK){
                free(msg);
                break;
            }
            else{
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

            while(threadsAvailable <= 0){
                pthread_cond_wait(&threads_cond, &threads_lock);
            }

            threadsAvailable--;
        pthread_mutex_unlock(&threads_lock);


        pthread_mutex_lock(&places_lock);

            while(placesAvailable <= 0){
                pthread_cond_wait(&places_cond, &places_lock);
            }

            placesAvailable--;
            msg->pl = getAvailablePlace(places, a.nplaces, msg->i);
        pthread_mutex_unlock(&places_lock);

        requestCounter ++;

        // Create thread to handle the request of the client
        if(pthread_create(&threads[threadNum], NULL, handle_request, msg) != OK){
            free(msg);
            fprintf(stderr,"Error creating thread.\n");
            break;
        }

        threadNum++;      

        if (threadNum % MAX_THREADS == 0){ // If reached the MAX limit of total threads
            // realloc more memory for threads
            int i = threadNum / MAX_THREADS + 1;
            threads = realloc(threads, i * MAX_THREADS * sizeof(pthread_t));
        } 
    }

    
    // Create thread to handle requests while server is closing
    pthread_t sclosing_thread;
    int server_opened = 1;
    pthread_create(&sclosing_thread, NULL, server_closing, &server_opened);

    // Wait for all threads to finish except the ones thrown when server was already closing
    for(int i = 0; i < threadNum; i++){
        pthread_join(threads[i],NULL);
    }

    if(unlink(fifoName) < 0){
        fprintf(stderr, "Error when destroying '%s'.\n",fifoName);
        exit(ERROR);
    }

    server_opened = 0; // To inform the thread that is closing requests that no more answers should be sent
    
    // Wait for the thread that is handling the requests sent when the server was closing
    pthread_join(sclosing_thread,NULL);
    
    free(threads);

    // Close the server and stop receiving requests
    close(server_fd);
        
    exit(OK);
}