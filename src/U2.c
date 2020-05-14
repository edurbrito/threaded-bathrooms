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

int fdserver = 0; // server file descriptor
int threadsAvailable = 50; // threads running at the same time / simultaneously

// Used to wait for available threads without busy waiting
pthread_mutex_t threads_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t threads_cond = PTHREAD_COND_INITIALIZER;

void incrementThreadsAvailable(){
    pthread_mutex_lock(&threads_lock);
    threadsAvailable++;
    pthread_cond_signal(&threads_cond);
    pthread_mutex_unlock(&threads_lock);
}

void * client_request(void * arg){

    int id = * (int *) arg;
    message msg;
    int fdread;

    char fifoClient[FIFONAME_SIZE + 50];
    sprintf(fifoClient, "/tmp/%d.%lu", getpid(), pthread_self());

    // Creates FIFO for the client to read info from server
    if(mkfifo(fifoClient, 0660) < 0){
        if (errno == EEXIST)
            printf("FIFO '%s' already exists.\n", fifoClient);
        else {
            fprintf(stderr,"Can't create FIFO '%s'.\n", fifoClient); 
            incrementThreadsAvailable();
            return NULL;
        }
    }

    buildMsg(&msg, id, fifoClient);

    // Sends message to server
    if((write(fdserver, &msg, sizeof(message))) == -1){
        fprintf(stderr,"Error sending message to Server.\n");
        logOP(FAILD,msg.i,msg.dur,msg.pl);
        incrementThreadsAvailable();
        return NULL;
    }

    logOP(IWANT,msg.i,msg.dur,msg.pl);

    if((fdread = open(fifoClient,O_RDONLY)) == -1){
        fprintf(stderr,"Error opening '%s' in READONLY mode.\n",fifoClient);
        logOP(FAILD,msg.i,msg.dur,msg.pl);
        incrementThreadsAvailable();
        return NULL;
    }

    // Receives message from server
    if (read(fdread, &msg, sizeof(message)) < 0) {
        fprintf(stderr, "Couldn't read from %d", fdread);
        logOP(FAILD,msg.i,msg.dur,msg.pl);
        incrementThreadsAvailable();
        return NULL;
    }

    if(msg.dur == -1 && msg.pl == -1)
        logOP(CLOSD,msg.i,msg.dur,msg.pl);
    else
        logOP(IAMIN,msg.i,msg.dur,msg.pl);

    close(fdread);

    incrementThreadsAvailable();

    if(unlink(fifoClient) < 0){
        fprintf(stderr, "Error when destroying '%s'.'\n",fifoClient);
        return NULL;
    }

    return NULL;
}

int main(int argc, char * argv[]){

    args a;

    if (checkArgs(argc, argv, &a, U) != OK ){
        fprintf(stderr,"Usage: %s <-t nsecs> fifoname\n",argv[0]);
        exit(ERROR);
    }

    if (ignoreSIGPIPE() != OK){
        fprintf(stderr,"Error ignoring SIGPIPE\n");
        exit(ERROR);
    }

    int threadNum = 0;
    pthread_t timechecker; // timechecker will check the time left
    pthread_t * threads = (pthread_t *) malloc(MAX_THREADS * sizeof(pthread_t));
    int * ids = (int *) malloc(MAX_THREADS * sizeof(int)); 

    srand((unsigned) time(NULL)); // init a random generator

    struct stat stat_buf;
    char filepath[FIFONAME_SIZE + 50];    
    sprintf(filepath,"/tmp/%s",a.fifoName);

    if (lstat(filepath, &stat_buf) == -1 || !S_ISFIFO(stat_buf.st_mode)){
        fprintf(stderr,"The file is not a FIFO...\n"); 
        free(threads);
        free(ids);       
        exit(ERROR);
    }

    if((fdserver = open(filepath, O_WRONLY)) == -1){
        fprintf(stderr,"Could not open the FIFO...\n");
        free(threads);
        free(ids);
        exit(ERROR);
    }

    int terminated[] = {a.nsecs, 0}; // 2nd element will be 1 if time is over, creating no more threads on main thread
    
    // Creating the synchronous thread that just checks if the time ended
    // Notifying the main thread with the @param terminated variable
    if(pthread_create(&timechecker, NULL, timeChecker, (void *) terminated) != OK){
        fprintf(stderr,"Error creating timeChecker thread.\n");
        free(threads);
        free(ids);
        exit(ERROR);
    }
    
    if(pthread_detach(timechecker) != OK){
        fprintf(stderr,"Error detaching timeChecker thread.\n");
        free(threads);
        free(ids);
        exit(ERROR);
    }

    // Here comes the creation of all the other threads;
    while(!terminated[1]){

        if (lstat(filepath, &stat_buf) == -1 || !S_ISFIFO(stat_buf.st_mode)){
            fprintf(stderr,"The file is not a FIFO...\n");        
            break;
        }

        pthread_mutex_lock(&threads_lock);

            while(threadsAvailable <= 0){
                pthread_cond_wait(&threads_cond, &threads_lock);
            }

            threadsAvailable--;
            
        pthread_mutex_unlock(&threads_lock);

        ids[threadNum] = threadNum;

        if(pthread_create(&threads[threadNum], NULL, client_request, (void *) &ids[threadNum])){
            fprintf(stderr,"Error creating thread.\n");
            break;
        }

        threadNum++;

        if (threadNum % MAX_THREADS == 0){ // If reached the MAX limit of total threads
            // realloc more memory for threads & ids
            int i = threadNum / MAX_THREADS + 1;
            threads = realloc(threads, i * MAX_THREADS * sizeof(pthread_t));
            ids = realloc(ids, i * MAX_THREADS * sizeof(int));
        }

        int r = 1 + rand() % 75;
        usleep(1000 * r); // Waiting a random number of milliseconds ranging between 1 ms and 25 ms
    }

    // Here comes the thread's joinings
    for (int i = 0; i < threadNum; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    free(ids);
    
    close(fdserver);
       
    exit(OK);
}