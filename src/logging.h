/**
 * @file logging.h
 * @author SOPE Group T02G05
 * @brief File containing logging' API headers and constants
 */

#ifndef LOGGING_H
#define LOGGING_H

typedef enum{ IWANT, RECVD, ENTER, IAMIN, TIMUP, TLATE, CLOSD, FAILD, GAVUP } action;

/**
 * @brief Appends some log info to the stdout
 * @param a the action performed to be logged 
 * @param i request order number 
 * @param dur duration in milliseconds of the request
 * @param pl place number gotten when requesting it
 * @return OK if successful, ERROR if not
*/
int logOP(action a, int i , int dur, int pl);

#endif