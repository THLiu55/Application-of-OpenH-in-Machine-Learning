
/*--------------------------------------------------------*/

/*
@file
@author Ravi Reddy Manumachu <ravi.manumachu@ucd.ie>
@version 1.0
*/

/*--------------------------------------------------------*/

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

/*--------------------------------------------------------*/

#include <openh.h>
#include <openhversion.h>
#include <openhhelpers.h>
#include <openherr.h>

/*--------------------------------------------------------*/

#define OPENHBUFFERLENGTH 2048
#define OPENHAFFINITYLIBRARYSTRING "OPENH HYBRID APP MODEL"

/*--------------------------------------------------------*/

int
openh_printf2(
   const char* format,
   const char* function,
   const int line,
   ...
)
{
    unsigned int verbosity = openh_get_verbosity();

    if (!verbosity)
    {
       return OPENH_SUCCESS;
    }

    char buffer[OPENHBUFFERLENGTH];
    va_list argptr;
    va_start(argptr, line);
    vsprintf(buffer, format, argptr);
    va_end(argptr);

    fprintf(
       stdout,
       "(%s) ==> (INFO) ==> %s:%d ==> %s\n",
       OPENHAFFINITYLIBRARYSTRING,
       function, line,
       buffer
    );

    return OPENH_SUCCESS;
}

/*--------------------------------------------------------*/

int
openh_err_printf2(
   const char* format,
   const char* function,
   const int line,
   ...
)
{
    char buffer[OPENHBUFFERLENGTH];
    va_list argptr;
    va_start(argptr, line);
    vsprintf(buffer, format, argptr);
    va_end(argptr);

    fprintf(
       stderr,
       "(%s) ==> (ERROR) ==> %s:%d ==> %s\n",
       OPENHAFFINITYLIBRARYSTRING,
       function, line,
       buffer
    );

    return OPENH_SUCCESS;
}

/*--------------------------------------------------------*/
