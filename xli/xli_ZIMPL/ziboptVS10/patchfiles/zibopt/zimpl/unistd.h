#ifndef _UNISTD_H 
#define _UNISTD_H
 
/* This file intended to serve as a drop-in replacement for  
 *  unistd.h on Windows 
 *  Please add functionality as neeeded  
 */ 
 
#include <stdlib.h> 
#include <io.h> 
#include <getopt.h> /* getopt from: http://www.pwilson.net/sample.html. */ 

//define the version of ZIMPL, which has been defined in Makefile
#ifndef VERSION
#define VERSION "3.1.0"
#endif

#define srandom srand 
#define random rand 
 
#define W_OK 2
#define R_OK 4 
 
#define access _access 
#define ftruncate _chsize 

#define ssize_t int 
 
#endif /* unistd.h  */ 

