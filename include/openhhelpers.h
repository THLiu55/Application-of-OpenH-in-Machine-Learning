
/*--------------------------------------------------------*/

/*
@file
@author Ravi Reddy Manumachu <ravi.manumachu@ucd.ie>
@version 1.0
*/

/*--------------------------------------------------------*/

#ifndef _OPENH_HELPERS_H_
#define _OPENH_HELPERS_H_

/*--------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------*/

#define openh_printf(format, ...) \
        openh_printf2(format, __func__, __LINE__, ##__VA_ARGS__)

/**
 * Helper function to print debug information.
 *
 * @param format The format string.
 * @param function The function name from where the comment is originating.
 * @param line The line number of this call
 *
 * return OPENH_SUCCESS on successful invocation.
 */
extern int
openh_printf2(
   const char* format,
   const char* function,
   const int line,
   ...
);

/*--------------------------------------------------------*/

#define openh_err_printf(format, ...) \
        openh_err_printf2(format, __func__, __LINE__, ##__VA_ARGS__)

/**
 * Helper function to print error information.
 *
 * @param format The format string.
 * @param function The function name from where the comment is originating.
 * @param line The line number of this call
 *
 * return OPENH_SUCCESS on successful invocation.
 */
extern int
openh_err_printf2(
   const char* format,
   const char* function,
   const int line,
   ...
);

/*--------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

/*--------------------------------------------------------*/

#endif /* _OPENH_HELPERS_H_ */

/*--------------------------------------------------------*/
