
/*--------------------------------------------------------*/

/*
@file
@author Ravi Reddy Manumachu <ravi.manumachu@ucd.ie>
@version 1.0
*/

/*--------------------------------------------------------*/

#include <stdio.h>

/*--------------------------------------------------------*/

#include "openherr.h"

/*--------------------------------------------------------*/

const char* openh_errMsg[] = {
"SUCCESS",
"sched_setaffinity error for master thread",
"sched_setaffinity error for accelerator component thread",
"sched_setaffinity error for CPU component thread",
"Out of memory error",
"popen error",
"tokenizer error",
"OPENH library internal error"
};

/*--------------------------------------------------------*/

const char*
openh_perror(
   const unsigned int errorCode
)
{
   return openh_errMsg[errorCode];
}

/*--------------------------------------------------------*/

