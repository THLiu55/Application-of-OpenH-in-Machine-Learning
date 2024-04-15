
/*--------------------------------------------------------*/

/*
@file
@author Ravi Reddy Manumachu <ravi.manumachu@ucd.ie>
@version 1.0
*/

/*--------------------------------------------------------*/

#ifndef _OPENH_VERSION_H_
#define _OPENH_VERSION_H_

/*--------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------*/

#define MAJOR_VERSION 1
#define MINOR_VERSION 0
extern const char* openh_parepopt_version_str;

/*--------------------------------------------------------*/

/**
 * Returns the version of openh library.
 *
 * @return The version of openh library
 */
extern const char*
openh_parepopt_get_version();

/*--------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

/*--------------------------------------------------------*/

#endif /* _OPENH_VERSION_H_ */

/*--------------------------------------------------------*/
