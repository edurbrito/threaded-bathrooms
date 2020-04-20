#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h> 
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "utils.h"
#include "types.h"
#include "logging.h"

void * clients_request(void * arg) { 

    clientParams *params = (struct clientParams *)arg;
    message msg;
    char fifoClient[FIFONAME_SIZE];
    int fdwrite, fdread;

    sprintf(fifoClient, "/tmp/%d.%lu", getpid(), pthread_self());

    // Creats fifo for the client to read info from server
    if(mkfifo(fifoClient,0660) < 0){
        if (errno == EEXIST)
            printf("FIFO '%s' already exists.\n",params->fifoName);
        else {
            fprintf(stderr,"Can't create FIFO %s.\n",params->fifoName); 
            //exit(ERROR);
        }
    }

    if((fdwrite = open(params->fifoName, O_WRONLY)) == -1){
        fprintf(stderr,"Error opening %s in WRITEONLY mode.\n",params->fifoName);
        if(unlink(fifoClient) < 0){
            fprintf(stderr, "Error when destroying %s.'\n",fifoClient);
        }
        return NULL;
    }

    buildMsg(&msg, params->id);

    strcpy(msg.fifoName, fifoClient);

    // Send message to server
    if((write(fdwrite, &msg, sizeof(message))) == -1){
        fprintf(stderr,"Error sending message to Server.\n");
        //exit(ERROR);
    }
    close(fdwrite);

    if((fdread = open(fifoClient,O_RDONLY)) == -1){
        fprintf(stderr,"Error opening %s in READONLY mode.\n",fifoClient);
        //exit(ERROR);
    }

    // Receive message from server
    if (read(fdread, &msg, sizeof(message)) < 0) {
        fprintf(stderr, "Couldn't read from %d", fdread);
        //exit(ERROR);
    }
    close(fdread);

    if(unlink(fifoClient) < 0){
        fprintf(stderr, "Error when destroying %s.'\n",fifoClient);
        exit(ERROR);
    }

    printMsg(&msg);

    return NULL;
}

int main(int argc, char * argv[]){

    args a;

    int client_thread_array_size = 0;

    clientParams * params = (clientParams *)malloc(sizeof(clientParams));

    pthread_t * client_thread_array = (pthread_t *)malloc(0);
    if (client_thread_array == NULL) {
        printf("Error! memory not allocated.");
        exit(ERROR);
    }

    if ( checkArgs(argc, argv, &a, U) != OK ){
        fprintf(stderr,"Usage: %s <-t nsecs> fifoname\n",argv[0]);
        logOP(CLOSD,1,12,2);
        exit(ERROR);
    }

    sprintf(params->fifoName,"/tmp/%s",a.fifoName);

    for (int i = 0 ; i < 3; i++) {

        params->id = i;

        client_thread_array = (pthread_t *) realloc(client_thread_array, (i + 1)*sizeof(pthread_t));
        if (client_thread_array == NULL) {
            fprintf(stderr,"Reallocation failed\n");
            exit(ERROR);
        }

        pthread_create(&client_thread_array[i], NULL, clients_request, (void *)params);

        client_thread_array_size++;
        usleep(5000000);
    }

    for (int i = 0; i < client_thread_array_size; i++) {
        pthread_join(client_thread_array[i], NULL);
    }

    /*if ( logOP(CLOSD,2,24,4) != OK )
        return ERROR;*/

    exit(OK);

}