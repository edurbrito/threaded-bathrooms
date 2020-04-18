#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
#include "types.h" 

void printArgs(args * a){
    printf("###### ARGS ######\n");
    printf("nsecs = %d\n", a->nsecs);
    printf("nplaces = %d\n", a->nplaces);
    printf("nthreads = %d\n", a->nthreads);
    printf("fifoname = %s\n", a->fifoname);
    printf("###### ARGS ######\n");
}

int checkArgs(int argc, char * argv[], args * a, caller C){
   
    int c;
    int option_index = 0;

    args tempargs;

    int nplaces = 0, nthreads = 0;

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
                nplaces = 1;
                break;

            case 'n':
                if( (tempargs.nthreads = atoi(optarg)) <= 0 || C != Q )
                    return ERROR;
                nthreads = 1;
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
    // In our case, it may represent the fifoname
    if (optind < argc){
        // printf("non-option ARGV-elements: ");
        while (optind < argc){
            sprintf(tempargs.fifoname, "%s", argv[optind++]);
        }
    }

    if( strlen(tempargs.fifoname) == 0 ) 
        return ERROR;

    a->nsecs = tempargs.nsecs;
    if ( C == Q ){
        if ( nplaces && nthreads ){
            a->nplaces = tempargs.nplaces;
            a->nthreads = tempargs.nthreads;
        }
        else
            return ERROR;
    }
    else{
        a->nplaces = -1;
        a->nthreads = -1;
    }
    strcpy(a->fifoname, tempargs.fifoname);
   
    return OK;
}