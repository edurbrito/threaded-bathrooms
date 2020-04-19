#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "utils.h"
#include "types.h"
#include "logging.h"

int main(int argc, char * argv[]){

    args a;

    if ( checkArgs(argc, argv, &a, U) != OK ){
        fprintf(stderr,"Usage: %s <-t nsecs> fifoname\n",argv[0]);
        logOP(CLOSD,1,12,2);
        exit(ERROR);
    }

    printArgs(&a);

    if (logOP(CLOSD,2,24,4) != OK )
        return ERROR;

    exit(OK);

}