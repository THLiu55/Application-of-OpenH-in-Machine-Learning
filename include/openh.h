/*--------------------------------------------------------*/

/*
 * @file
 * @author Ravi Reddy Manumachu <ravi.manumachu@ucd.ie>
 * @version 1.0
 */
/*--------------------------------------------------------*/

#ifndef _OPENH_H_
#define _OPENH_H_

/*
 * OpenH is a novel programming model and library API for 
 * developing portable parallel programs on heterogeneous 
 * hybrid servers composed of a multicore CPU and one or 
 * more different types of accelerators. 
 *
 * OpenH integrates Pthreads, OpenMP, and OpenACC seamlessly 
 * to facilitate the development of hybrid parallel programs. 
 * An OpenH hybrid parallel program starts as a single main 
 * thread, which then creates a group of Pthreads called 
 * hosting Pthreads. A hosting Pthread then leads the 
 * execution of a software component of the program, which 
 * is either an OpenMP multithreaded component running on 
 * the CPU cores or an OpenACC (or OpenMP) component running 
 * on one of the accelerators of the server. The OpenH library 
 * provides API functions that allow programmers to get the 
 * configuration of the executing environment and bind the 
 * hosting Pthreads (and hence the execution of components) 
 * of the program to the CPU cores of the hybrid server 
 * to get the best performance.
 *
 * The API is described in the following article:
 * OpenH: A Novel Programming Model and API for 
 * Developing Portable Parallel Programs on Heterogeneous Hybrid Servers
 */
/*--------------------------------------------------------*/

enum openh_mapping_scheme {
   OPENH_L2P_ROUNDROBIN = 0,
   OPENH_L2P_LINEAR,
   OPENH_L2P_NWK
};

enum openh_acc_type {
   OPENH_CUDA_GPU = 0,
   OPENH_AMD_GPU,
   OPENH_FPGA,
   OPENH_UNKNOWN
};

/*--------------------------------------------------------*
 *           
 *  API for Environment and Topology
 *
 *--------------------------------------------------------*/

/*
 * The OpenH library runtime is initialized and destroyed 
 * using the following API functions.
 *
 * The above functions must only be invoked by the main thread. 
 * It is erroneous to call any other OpenH library function 
 * before openh_init().
 */
extern int openh_init();
extern int openh_finalize();

/*
 * Functions to print debug information.
 */
extern int openh_print();
extern unsigned int openh_get_verbosity();
extern void openh_set_verbosity(
       unsigned int verbosity);

/*
 * The following function returns 1 if hyperthreading is enabled 
 * on the platform and 0 otherwise. 
 * The function is reentrant and thread-safe.
 */
extern int openh_is_hyperthreaded();

/*
 * The API functions, openh_get_num_pcores() and openh_get_num_lcores(),
 * return the number of physical and logical CPU cores in the platform, respectively.
 * Both functions are reentrant and thread-safe.
 */
extern int openh_get_num_pcores();
extern int openh_get_num_lcores();

/*
 * If hyperthreading is not present, the number of physical
 * CPU cores will be the same as the number of logical CPU
 * cores.
 * If hyperthreading is present, the number of logical
 * cores will be greater than the number of physical cores.
 * For this case, the OpenH library provides two API
 * functions for obtaining the mapping between the OpenH
 * logical CPU core IDs to the OpenH physical CPU
 * core IDs.
 * The first API function, openh_get_mapping_scheme(),
 * returns the underlying hardware vendor’s scheme for map-
 * ping logical CPU core IDs to physical CPU core IDs. The
 * function is reentrant and thread-safe.
 */
extern int openh_get_mapping_scheme();

/*
 * The  second  API  function,  openh_get_mapping_function(),
 * returns the mapping function given the input 
 * mapping scheme, the mapping_scheme. 
 * The programmer first obtains the mapping scheme using 
 * openh_mapping_scheme() and then uses this scheme to obtain 
 * the physical-to-logical function pointer, openh_ptol_func, using 
 * the function openh_get_ptol_function(). 
 * Finally, the programmer obtains the list of OpenH logical CPU core IDs 
 * in the array, lcpuids, using the function pointer, openh_ptol_func, for 
 * the input physical CPU core ID, openh_pcoreid. 
 * The size of the array lcoreids is nlids. 
 * Memory for lcoreids is obtained using malloc() and can be freed with free().
 */
typedef int ( *openh_pcoreid_func )(
            int openh_lcoreid);
extern openh_pcoreid_func openh_get_mapping_function(
            int mapping_scheme);

/*
 * The API function, openh_get_ptol_function(), allows the programmer 
 * to obtain the list of OpenH logical CPU core IDs associated with 
 * an OpenH physical CPU core ID. It is reentrant and thread-safe.
 */
typedef int ( *openh_ptol_func )(
            int openh_pcoreid, int** lcpuids, int* nlids);
extern openh_ptol_func openh_get_ptol_function(
            int mapping_scheme);

/*--------------------------------------------------------*
 *           
 *  API for Accelerators
 *
 *--------------------------------------------------------*/

/*
 * The API function, openh_get_num_accelerators(), returns the 
 * number of accelerators in the execution platform.
 */
extern int openh_get_num_accelerators();

 /*
  * The API function, openh_get_acc_type(), returns the type of 
  * the accelerator given the id.
  * The API functions, openh_get_num_accelerators() and 
  * openh_get_acc_type(), together allow the programmer to obtain 
  * a mapping between the accelerator IDs and types. 
  * Both functions are reentrant and thread-safe.
  */
extern int openh_get_acc_type(int id);

/*
 * The OpenH physical CPU core IDs closest to an accelerator can be 
 * obtained using the API function openh_get_accelerator_pcpuaffinity. 
 * This function returns the closest OpenH physical CPU core IDs 
 * to the accelerator, accnum, in the array, closestPcpuids. 
 * The size of the output array is npcpuids. 
 * Memory for closestPcpuids is obtained using malloc() in the API function, 
 * and can be freed with free(). 
 * Similarly, the closest OpenH logical CPU core IDs can be obtained 
 * using the API function openh_get_accelerator_lcpuaffinity.
 */
extern int openh_get_accelerator_lcpuaffinity(
       int accnum,
       int** closestCpuids,
       int* ncpus);
extern int openh_get_accelerator_pcpuaffinity(
       int accnum,
       int** closestCpuids,
       int* ncpus);

/*
 * The OpenH library provides two high-level functions that 
 * allow the programmer to assign unique OpenH physical CPU 
 * core IDs or OpenH logical CPU core IDs that map to unique 
 * OpenH physical CPU core IDs. The API function, openh_get_unique_pcore(), 
 * assigns a unique OpenH physical CPU core ID to pin the hosting Pthread 
 * for the accelerator accId. Finally, the API function, openh_get_unique_lcore(), 
 * assigns a unique OpenH logical CPU core ID to pin the hosting Pthread 
 * for the accelerator accId. Furthermore, different accelerator hosting Pthreads 
 * are assigned unique OpenH logical CPU core IDs that map to a unique 
 * OpenH physical CPU core ID when using the API function, openh_get_unique_lcore().
 *
 * The two API functions, openh_get_unique_pcore() and openh_get_unique_lcore(), 
 * are not thread-safe. It is recommended that programmers invoke these functions 
 * only in the main thread. Furthermore, only one of the functions must be used 
 * for obtaining the unique OpenH CPU core IDs to assign for binding the accelerator hosting Pthreads.
 */
extern int openh_get_unique_pcore(int accId);
extern int openh_get_unique_lcore(int accId);

/*--------------------------------------------------------*
 *           
 *  API for assigning CPU core IDs
 *
 *--------------------------------------------------------*/

/*
 * The API function, openh_assign_main_pcpuid, assigns the OpenH 
 * physical CPU core ID, pcpuid, for binding the main thread. 
 * It must be invoked by the main thread only.
 *
 * The API function, openh_assign_acc_pcpuids, assigns the OpenH 
 * physical CPU core IDs provided in the array, pcpuids, for binding 
 * the hosting Pthread for the accelerator accId. 
 * The size of the array is provided in the size argument.
 *
 * The API function, openh_assign_cpu_pcpuids, assigns the 
 * OpenH physical CPU core IDs provided in the array, pcpuids, for 
 * binding the CPU hosting Pthread and the execution of the CPU component, cpuComponentId. 
 * The size of the array is provided in the size argument.
 */
extern void openh_assign_main_pcpuid(int pcpuid);

extern void openh_assign_acc_pcpuids(
     int accId,
     int *pcpuids,
     int size);

extern void openh_assign_cpu_pcpuids(
     int cpuComponentid,
     int *pcpuids,
     int size);

/*
 * The following assign the OpenH logical CPU core IDs for 
 * binding the main and hosting Pthreads.
 */
extern void openh_assign_main_lcpuid(int lcpuid);

extern void openh_assign_acc_lcpuids(
     int accId,
     int *lcpuids,
     int size);

extern void openh_assign_cpu_lcpuids(
     int cpuComponentid,
     int *lcpuids,
     int size);

/*
 * The following API functions are helpers for automatically assigning 
 * the OpenH physical and logical CPU core IDs for binding the CPU hosting Pthread 
 * and the execution of the CPU component, cpuComponentId.
 */
extern void openh_assign_cpu_free_pcpuids(
     int cpuComponentid);

extern void openh_assign_cpu_free_lcpuids(
     int cpuComponentid);

/*--------------------------------------------------------*
 *           
 *  API for binding software components to CPU core IDs
 *
 *--------------------------------------------------------*/

/*
 * The following binding API functions actually bind the 
 * hosting Pthreads to the CPU core IDs that have been assigned 
 * using the affinity API functions.
 *
 * The function openh_bind_main_self() must be called by the main thread only. 
 * The function openh_bind_acc_self() is called by the hosting Pthread for the 
 * accelerator accId. It binds the accelerator hosting Pthread to the core IDs 
 * assigned for binding in the API function calls, openh_assign_acc_pcpuids() or openh_assign_acc_lcpuids().
 *
 * The function openh_bind_cpu_self() is called by the hosting Pthread for 
 * the CPU component cpuComponentId. It binds the CPU hosting Pthread and 
 * the execution of the CPU component to the CPU core IDs assigned for binding 
 * in the API function calls, openh_assign_cpu_pcpuids() or openh_assign_cpu_lcpuids(). 
 * Therefore, the CPU component executes only on the CPU core IDs assigned using the affinity assignment API.
 *
 * Finally, these functions are typically the first invocations in the Pthread function 
 * execution of the corresponding component. 
 * For example, the function openh_bind_cpu_self() would be invoked before any processing  
 * in the Pthread function associated with a CPU component.
 */
extern int openh_bind_main_self();

extern int openh_bind_acc_self(int accId);

extern int openh_bind_cpu_self(int cpuComponentId);

/*--------------------------------------------------------*/

#endif /* _OPENH_H_ */

/*--------------------------------------------------------*/

