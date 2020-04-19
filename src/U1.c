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

    struct clientparameters *parameters = (struct clientparameters *)arg;
    struct message msg;
    char fifoclient[40];
    int fdwrite, fdread;

    sprintf(fifoclient, "/tmp/fifo_cliente_%d.%ld", getpid(), pthread_self());

    //Cria o fifo para o cliente ler a informacao do servidor
    if(mkfifo(fifoclient,0660) < 0){
        if (errno == EEXIST)
            printf("FIFO '%s' already exists.\n",parameters->fifoname);
        else {
            printf("Can't create FIFO %s.\n",parameters->fifoname); 
            exit(ERROR);
        }
    }

    //Abre o ficheiro para escrever ao servidor o pedido
    if ((fdwrite = open(parameters->fifoname, O_WRONLY)) != -1)
        printf("FIFO '%s' openned in WRITEONLY mode\n",parameters->fifoname);

    msg.dur = (rand() % 2) + 1;
    msg.pid = getpid();
    msg.tid = pthread_self();
    msg.i = parameters->id;
    strcpy(msg.fifoname, fifoclient);

    write(fdwrite, &msg, sizeof(struct message));
    close(fdwrite);

    if ((fdread = open(fifoclient,O_RDONLY)) != -1)
        printf("FIFO %s openned in READONLY mode\n", fifoclient);

    //Abre o ficheiro para ler do servidor a resposta
    if (read(fdread, &msg, sizeof(struct message)) != OK) {
        fprintf(stderr, "Couldn't read from %d", fdwrite);
        exit(ERROR);
    }

    if (msg.pl == -1) {
        printf("Numero do pedido = %d\n", msg.i);
        printf("Pid = %d\n", msg.pid);
        printf("Tid = %ld\n", msg.tid);
        printf("Duracao = %d\n", msg.dur);
        printf("Lugar atribuido = %d\n", msg.pl);
        printf("Bathroom closed\n");
    }
    else {
        printf("Numero do pedido = %d\n", msg.i);
        printf("Pid = %d\n", msg.pid);
        printf("Tid = %ld\n", msg.tid);
        printf("Duracao = %d\n", msg.dur);
        printf("Lugar atribuido = %d\n", msg.pl);
    }

    return NULL;
}

int main(int argc, char * argv[]){

    args a;

    int client_thread_array_size = 0;

    struct clientparameters * parameters = (struct clientparameters *)malloc(sizeof(struct clientparameters));

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

    strcpy(parameters->fifoname, a.fifoname);

    for (int i = 0 ; i < 3; i++) {

        usleep(1000);

        parameters->id = i;

        client_thread_array = (pthread_t *) realloc(client_thread_array, (i + 1)*sizeof(pthread_t));
        if (client_thread_array == NULL) {
            fprintf(stderr,"Reallocation failed\n");
            exit(ERROR);
        }

        pthread_create(&client_thread_array[i], NULL, clients_request, (void *)parameters);

        client_thread_array_size++;
    }

    for (int i = 0; i < client_thread_array_size; i++) {
        pthread_join(client_thread_array[i], NULL);
    }


    /*if ( logOP(CLOSD,2,24,4) != OK )
        return ERROR;*/

    exit(OK);

}