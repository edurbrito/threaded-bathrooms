#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h> 
#include <fcntl.h> // For O_* constants
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/mman.h>
#include <sys/types.h> 
#include "utils.h"
#include "types.h"

int fdserver = 0; // Server FIFO

// To inform the Client process that the Server will not send any more responses to requests
Shared_memory *shmem; 

void * client_request(void * arg){

    int id = * (int *) arg;
    message msg;
    int fdread;

    char fifoClient[FIFONAME_SIZE + 50];
    sprintf(fifoClient, "/tmp/%d.%lu", getpid(), pthread_self());

    // Creates FIFO for the client to read info from server
    if(mkfifo(fifoClient, 0660) < 0){
        if (errno == EEXIST)
            fprintf(stderr,"FIFO '%s' already exists.\n", fifoClient);
        else {
            fprintf(stderr,"Can't create FIFO '%s'.\n", fifoClient); 
            return NULL;
        }
    }

    if((fdread = open(fifoClient,O_RDONLY|O_NONBLOCK)) == -1){
        fprintf(stderr,"Error opening %s in READONLY mode.\n",fifoClient);
        return NULL;
    } 

    buildMsg(&msg, id, fifoClient);

    // Sends message to server
    if((write(fdserver, &msg, sizeof(message))) == -1){
        fprintf(stderr,"Error sending message to Server.\n");
        logOP(FAILD,msg.i,msg.dur,msg.pl);
        return NULL;
    }

    logOP(IWANT,msg.i,msg.dur,msg.pl);      
    
    // Try to read answer from server while the server is still sending answers
    int r = 0;
    
    // Try to read at least one time. Stop trying if the server as sent all answers already
    do{
        r = read(fdread, &msg, sizeof(message));

        // No message to read yet or error
        if(r <= 0){
            // Error that does not concern the NON BLOCKING MODE -> EXIT
            if(r < 0 && isNotNonBlockingError() == OK)
                break;
             // Nothing to read
            else
                continue;
        }
        else break;
    } while(shmem->requests_pending == 0);

    // No more answers are being sent by the server and this request did not receive an answer
    if(r <= 0){
        // Try one more time
        r = read(fdread, &msg, sizeof(message));

        // No message to read yet or error
        if(r <= 0){
            fprintf(stderr, "Couldn't read from %d", fdread);
            logOP(FAILD,msg.i,msg.dur,msg.pl);
        }
        else{
            if(msg.dur == -1 && msg.pl == -1)
                logOP(CLOSD,msg.i,msg.dur,msg.pl);
            else
                logOP(IAMIN,msg.i,msg.dur,msg.pl);
        }
    }
    else{
        if(msg.dur == -1 && msg.pl == -1)
            logOP(CLOSD,msg.i,msg.dur,msg.pl);
        else
            logOP(IAMIN,msg.i,msg.dur,msg.pl);
    }

    close(fdread);

    if(unlink(fifoClient) < 0){
        fprintf(stderr, "Error when destroying '%s'.'\n",fifoClient);
        return NULL;
    }

    //printMsg(&msg);

    return NULL;
}

int main(int argc, char * argv[]){

    args a;

    // Check the arguments
    if (checkArgs(argc, argv, &a, U) != OK ){
        fprintf(stderr,"Usage: %s <-t nsecs> fifoname\n",argv[0]);
        exit(ERROR);
    }

    ignoreSIGPIPE();

    // Attach shared memory created by the server
    if ((shmem = attach_shared_memory(SHM_NAME, sizeof(int))) == NULL){
        perror("CLIENT: could not attach shared memory");
        exit(ERROR);
    }

    int threadNum = 0;
    pthread_t threads[MAX_THREADS], timechecker; // timechecker will check the time left
    int ids[MAX_THREADS]; 

    srand((unsigned) time(NULL)); // init a random generator

    struct stat stat_buf;
    char filepath[FIFONAME_SIZE + 50];    
    sprintf(filepath,"/tmp/%s",a.fifoName);

    if (lstat(filepath, &stat_buf) == -1 || !S_ISFIFO(stat_buf.st_mode)){
        fprintf(stderr,"The file is not a FIFO...\n");        
        exit(ERROR);
    }

    if((fdserver = open(filepath, O_WRONLY)) == -1){
        fprintf(stderr,"Could not open the FIFO...\n");
        exit(ERROR);
    }

    int terminated[] = {a.nsecs, 0}; // Second element will be 1 if time is over, creating no more threads on main thread
    
    // Creating the synchronous thread that just checks if the time ended
    // Notifying the main thread with the @p terminated variable
    pthread_create(&timechecker, NULL, timeChecker, (void *) terminated);
    pthread_detach(timechecker);

    // Here comes the creation of all the other threads;
    while(!terminated[1]){

        if(threadNum == MAX_THREADS){
            fprintf(stderr,"MAX THREADS exceeded...\n");
            break;
        }

        ids[threadNum] = threadNum;

        if (lstat(filepath, &stat_buf) == -1 || !S_ISFIFO(stat_buf.st_mode)){
            fprintf(stderr,"The file is not a FIFO...\n");        
            exit(ERROR);
        }

        if(pthread_create(&threads[threadNum], NULL, client_request, (void *) &ids[threadNum])){
            fprintf(stderr,"Error creating thread.\n");
            break;
        }

        threadNum++;

        int r = 1 + rand() % 250;
        usleep(1000 * r); // Waiting a random number of milliseconds ranging between 1 ms and 250 ms
    }

    //printf("Time is over... Threads will be joined.\n");

    // Here comes the thread's joinings
    for (int i = 0; i < threadNum; i++) {
        pthread_join(threads[i], NULL);
    }
    
    close(fdserver);

    if (munmap(shmem,sizeof(int)) < 0){
        perror("Failure in munmap()");
        exit(ERROR);
    } 

    exit(OK);
}