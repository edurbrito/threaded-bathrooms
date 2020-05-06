/**
 * @file utils.h
 * @author SOPE Group T02G05
 * @brief File containing utilities' headers and constants
 */

#ifndef UTILS_H
#define UTILS_H

#include "types.h"

/**
 * @brief Print args captured with @see checkArgs()
 * @param a args to be printed
 * @note For Debugging Purposes
*/
void printArgs(args * a);

/** 
 * @brief Fills the @p args, by checking the arguments passed to the function
 * @param argc number of args passed
 * @param argv args passed
 * @param a struct to be filled with args
 * @param C caller program
 * @return OK if successful, ERROR otherwise
 * @note Lots of Credits to those who managed to give a very good explanation at 
 * @see https://www.gnu.org/software/libc/manual/html_node/Getopt-Long-Option-Example.html
 * @see https://linux.die.net/man/3/getopt_long for more info on the functions used
 */
int checkArgs(int argc, char * argv[], args * a, caller C);

/**
 * @brief Appends some log info to the stdout
 * @param a the action performed to be logged 
 * @param i request order number 
 * @param dur duration of the request
 * @param pl place number gotten when requesting it
 * @return OK if successful, ERROR if not
*/
int logOP(action a, int i , int dur, int pl);

/**
 * @brief Thread function to measure remaining time for the program
 * that invoked it to terminate.
 * @param arg The argument passed as a void pointer, corresponding to a 
 * 2 elements' array, where the first is the number of seconds to
 * wait and the second element is the terminating variable, set to
 * one if the program is to be ended
*/
void * timeChecker(void * arg);

/** 
 * @brief Build the message to be sent to the Server
 * @param msg the message to fill with the correct parameters
 * @param id the number of the Client's request
 * @param fifoClient FIFO Client Path
 */
void buildMsg(message * msg, int id, char * fifoClient);

/** 
 * @brief Print message sent from client to server or from server to cliente (for testing purposes)
 * @param msg the message to print
 * @note For Debugging Purposes
 */
void printMsg(message * msg);

/** 
 * @brief Check if the error was caused by the fact that the FIFO is Non Blocking FIFO
 * @return OK if the error is caused by the Non Blocking FIFO
 */
int isNotNonBlockingError();

/**
 * @brief Ignores SIGPIPE in relation to the shared FIFO 
 * between the Client and the Server process
 * @return OK if successfull, ERROR otherwise
*/
int ignoreSIGPIPE();

#endif
