#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h> 
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include "utils.h"
#include "types.h"
#include "logging.h"

void * clients_request(void * arg) { 

    clientParams *params = (struct clientParams *)arg;
    message msg;
    char fifoClient[FIFONAME_SIZE];
    int fdread;

    sprintf(fifoClient, "/tmp/%d.%lu", getpid(), pthread_self());

    // Creats fifo for the client to read info from server
    if(mkfifo(fifoClient,0660) < 0){
        if (errno == EEXIST)
            printf("FIFO '%s' already exists.\n",params->fifoName);
        else {
            fprintf(stderr,"Can't create FIFO %s.\n",params->fifoName); 
            return NULL;
        }
    }

    buildMsg(&msg, params->id);

    strcpy(msg.fifoName, fifoClient);

    // Send message to server
    if((write(params->fdwrite, &msg, sizeof(message))) == -1){
        fprintf(stderr,"Error sending message to Server.\n");
        return NULL;
    }

    if((fdread = open(fifoClient,O_RDONLY)) == -1){
        fprintf(stderr,"Error opening %s in READONLY mode.\n",fifoClient);
        return NULL;
    }

    // Receive message from server
    if (read(fdread, &msg, sizeof(message)) < 0) {
        fprintf(stderr, "Couldn't read from %d", fdread);
        return NULL;
    }
    close(fdread);

    if(unlink(fifoClient) < 0){
        fprintf(stderr, "Error when destroying %s.'\n",fifoClient);
        return NULL;
    }

    printMsg(&msg);

    return NULL;
}

int main(int argc, char * argv[]){

    args a;

    if (checkArgs(argc, argv, &a, U) != OK ){
        fprintf(stderr,"Usage: %s <-t nsecs> fifoname\n",argv[0]);
        logOP(CLOSD,1,12,2);
        exit(ERROR);
    }

    int id = 0, threadNum = 0;

    clientParams * params = (clientParams *)malloc(sizeof(clientParams));

    pthread_t * threads = (pthread_t *)malloc(0);
    if (threads == NULL) {
        printf("Error! memory not allocated.");
        exit(ERROR);
    }

    sprintf(params->fifoName,"/tmp/%s",a.fifoName);

    time_t t = time(NULL) + a.nsecs;
    struct stat stat_buf;

    if((params->fdwrite = open(params->fifoName, O_WRONLY)) == -1){
        free(params);
        exit(ERROR);
    }

    while(time(NULL) < t){

        params->id = id;
        id++;

        threads = (pthread_t *) realloc(threads, (threadNum + 1)*sizeof(pthread_t));
        if (threads == NULL) {
            fprintf(stderr,"Reallocation failed\n");
            exit(ERROR);
        }

        if (lstat(params->fifoName, &stat_buf) == -1){
            break;
        }
        if(!S_ISFIFO(stat_buf.st_mode)){
            break;
        }
           

        if(pthread_create(&threads[threadNum], NULL, clients_request, (void *)params)){
            fprintf(stderr,"Error creating thread.\n");
            break;
        }

        threadNum++;
        usleep(5000);
    }

    for (int i = 0; i < threadNum; i++) {
        pthread_join(threads[i], NULL);
    }

    close(params->fdwrite);

    free(threads);
    free(params);

    /*if ( logOP(CLOSD,2,24,4) != OK )
        return ERROR;*/

    exit(OK);

}