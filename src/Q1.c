#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "utils.h"
#include "types.h"
#include "logging.h"

int main(int argc, char * argv[]){

    args a;

    if (checkArgs(argc, argv, &a, Q) != OK ){
        fprintf(stderr,"Usage: %s <-t nsecs> [-l nplaces] [-n nthreads] fifoname\n",argv[0]);
        logOP(CLOSD,1,12,2);
        exit(ERROR);
    }

    printArgs(&a);

    char fifoName[FIFONAME_SIZE+5];
    sprintf(fifoName,"/tmp/%s",a.fifoname);

    if(createFifo(fifoName) != OK){
        exit(ERROR);
    }

    // The real program goes here

    if(killFifo(fifoName) != OK){
        exit(ERROR);
    }

    /*if ( logOP(CLOSD,2,24,4) != OK )
        return ERROR;*/

    exit(OK);

}