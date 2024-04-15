
/*--------------------------------------------------------*/

/*
 * @file
 * @author Ravi Reddy Manumachu <ravi.manumachu@ucd.ie>
 * @version 1.0
 */
/*--------------------------------------------------------*/

#ifndef _OPENH_INTERNAL_H_
#define _OPENH_INTERNAL_H_

/*--------------------------------------------------------*/

int openh_get_physical_cpuids(
    int* num_physical_cpuids,
    int** physical_cpuids);

int openh_get_logical_cpuids(
    int* num_logical_cpuids,
    int** logical_cpuids);

int openh_siblings_list_first(
    int lcpuid, int* pcpuid);

int openh_ptol_siblings_func(
    int openh_pcoreid, int** lcpuids, int* nlids
);

int openh_roundrobin_pcoreid_func(int openh_lcoreid);

/*--------------------------------------------------------*/

#endif /* _OPENH_INTERNAL_H_ */

/*--------------------------------------------------------*/

