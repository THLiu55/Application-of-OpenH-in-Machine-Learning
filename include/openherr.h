
/*--------------------------------------------------------*/

/*
@file
@author Ravi Reddy Manumachu <ravi.manumachu@ucd.ie>
@version 1.0
*/

/*--------------------------------------------------------*/

#ifndef _OPENH_ERR_H_
#define _OPENH_ERR_H_

/*--------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------*/

#define OPENH_SUCCESS                        0
#define OPENH_ERR_PINMASTER                  1
#define OPENH_ERR_PINACCCID                  2
#define OPENH_ERR_PINCPUCID                  3
#define OPENH_ERR_NOMEM                      4
#define OPENH_ERR_POPEN                      5
#define OPENH_ERR_TOKEN                      6
#define OPENH_ERR_INTERNAL                   7

/*--------------------------------------------------------*/

extern
const char*
openh_perror(
   const unsigned int errorCode
);

/*--------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

/*--------------------------------------------------------*/

#endif /* _OPENH_ERR_H_ */

/*--------------------------------------------------------*/
