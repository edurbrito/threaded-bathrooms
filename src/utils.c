#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h> 
#include <string.h>
#include <time.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include "utils.h"
#include "types.h" 

void printArgs(args * a){
    printf("###### ARGS ######\n");
    printf("nsecs = %d\n", a->nsecs);
    printf("nplaces = %d\n", a->nplaces);
    printf("nthreads = %d\n", a->nthreads);
    printf("fifoName = %s\n", a->fifoName);
    printf("###### ARGS ######\n\n");
}

int checkArgs(int argc, char * argv[], args * a, caller C){
   
    int c;
    int option_index = 0;

    args tempargs;
    tempargs.nsecs = -1;
    tempargs.nplaces = 50;
    tempargs.nthreads = 50;
    tempargs.fifoName[0] = '\0';

    while(1) {         

        // The short_options array, as a string, is interpreted 
        // as a no_argument option for every alone character and 
        // a required_argument option for every character followed by a colon (:)
        c = getopt_long(argc, argv, "t:l:n:", NULL, &option_index);
        // c contains the current arg from argv that is being analyzed

        if (c == -1)
            break;
        
        switch (c){

            case 0: // This is a default option as found in the legit Linux Man Pages source 
                // printf("option %s", long_options[option_index].name);
                // if (optarg)
                    // printf(" with arg %s", optarg);
                // printf("\n");
                break;

            case 't':
                if( (tempargs.nsecs = atoi(optarg)) <= 0 )
                    return ERROR;
                break;

            case 'l':
                if( (tempargs.nplaces = atoi(optarg)) <= 0 || C != Q )
                    return ERROR;
                break;

            case 'n':
                if( (tempargs.nthreads = atoi(optarg)) <= 0 || C != Q )
                    return ERROR;
                break;
            
            case '?': // Unkown option in argv
                /* getopt_long already printed an error message. */
                // printf("Exiting...\n");
                return ERROR;

                break;

            default: // Some other error may have occurred 

                printf("Error: getopt returned character code 0%o \n", c);
                return ERROR;

                break;
        }
    }

    // This reveals non-option args, like loose strings
    // In our case, it may represent the fifoName
    if (optind < argc){
        // printf("non-option ARGV-elements: ");
        while (optind < argc){
            sprintf(tempargs.fifoName, "%s", argv[optind++]);
        }
    }

    if( strlen(tempargs.fifoName) == 0 ) 
        return ERROR;

    if( tempargs.nsecs <= 0)
        return ERROR;

    a->nsecs = tempargs.nsecs;

    a->nplaces = tempargs.nplaces;
    a->nthreads = tempargs.nthreads;
  
    strcpy(a->fifoName, tempargs.fifoName);
   
    return OK;
}

int logOP(action a, int i , int dur, int pl){
    
    char *actions[] = { "IWANT", "RECVD", "ENTER", "IAMIN", "TIMUP", "2LATE", "CLOSD", "FAILD", "GAVUP"};

    int pid = getpid();
    long int tid = pthread_self();

    char str[100];

    time_t seconds; 
    if( time(&seconds) == -1 )
        return ERROR; 

    if( sprintf(str, "%ld ; %06d ; %06d ; %ld ; %06d ; %06d ; %s\n", seconds, i, pid, tid, dur, pl, actions[a]) < 0 )
        return ERROR;

    if( write(STDOUT_FILENO, str, strlen(str)) != strlen(str) )
        return ERROR;

    return OK;
}

void * timeChecker(void * arg){

    // 1st element is the number of seconds to wait
    // 2nd element is to be set when terminated, for ending the while loop of the main thread
    int * terminated = (int *) arg; 

    int nsecs = terminated[0];
    sleep(nsecs);

    terminated[1] = 1;
    
    return NULL;
}

void buildMsg(message * msg, int id, char * fifoClient){
    
    msg->i = id;
    msg->pid = getpid();
    msg->tid = pthread_self();
    int r = 1 + rand() % 250; // From 1 to 250
    msg->dur = r; // miliseconds
    msg->pl = -1;

    strcpy(msg->fifoName, fifoClient);
}

void printMsg(message * msg){
    printf("###### MESSAGE ######\n");
    printf("Numero do pedido = %d\n", msg->i);
    printf("Pid = %d\n", msg->pid);
    printf("Tid = %ld\n", msg->tid);
    printf("Duracao = %d\n", msg->dur);
    printf("Lugar atribuido = %d\n", msg->pl);
    if(msg->pl == -1)
        printf("Bathroom closed\n");
    printf("###### MESSAGE ######\n\n");

}

int isNotNonBlockingError(){
    if(errno != EAGAIN && errno != EWOULDBLOCK){
        perror("Error in read\n");
        return OK;
    }
    return ERROR;
}

int ignoreSIGPIPE(){
    struct sigaction action;
    action.sa_handler = SIG_IGN;;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (sigaction(SIGPIPE, &action, NULL) < 0){
        fprintf(stderr,"Unable to install SIG handler\n");
        return ERROR;
    }

    return OK;
}

int getAvailablePlace(int places[], int size, int requestNum){
    int place = requestNum % size;

    if(places[place] == 0)
        return place;

    for(int i = 0 ;  i < size; i++){
        if(places[i] == 0){
            places[i] = 1;
            return i;
        }
    }
    return -1;
}

void freePlace(int places[], int placeNum){
    places[placeNum] = 0;
}
