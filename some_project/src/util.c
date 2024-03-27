#include <stdio.h>
#include "util.h"

/** 
 * Function: print
 * ---------------
 * Prints a string given a pointer to string using a simple printf function call
 * Does not do any checking
 * 
 * buf: Pointer to string
 * 
 * returns: void
*/
void print(const char *buf) {
    printf("%s\n", buf);
}
