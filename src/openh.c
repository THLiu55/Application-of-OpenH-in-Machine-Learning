/*--------------------------------------------------------*/

/*
 * @file
 * @author Ravi Reddy Manumachu <ravi.manumachu@ucd.ie>
 * @version 1.0
 */
/*--------------------------------------------------------*/

#define _GNU_SOURCE // sched_getcpu(3) is glibc-specific (see the man page)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <unistd.h>
#include <errno.h>
#include <hwloc.h>

/*--------------------------------------------------------*/

#ifdef _OPENMP
#include <omp.h>
#endif

#ifdef _OPENACC
#include <openacc.h>
#endif

/*--------------------------------------------------------*/

#include <openh.h>
#include <openherr.h>
#include <openhhelpers.h>
#include <openhversion.h>

/*--------------------------------------------------------*/

#include "openhinternal.h"

/*--------------------------------------------------------*/

cpu_set_t main_cpuset;
cpu_set_t* accelerator_cpusets;
cpu_set_t* cpu_cpusets;

int num_acc_components, num_cpu_components;

int num_logical_cpuids_global;
int* logical_cpu_pinned;

int num_physical_cpuids_global;
int* physical_cpu_pinned;

int* logical_to_physical_map;

unsigned int openh_verbosity;

/*--------------------------------------------------------*/

int openh_init()
{
    CPU_ZERO(&main_cpuset);

    int numaccelerators = openh_get_num_accelerators();

    accelerator_cpusets = (cpu_set_t*)malloc(
                           sizeof(cpu_set_t)*numaccelerators);

    if (accelerator_cpusets == NULL)
    {
       openh_err_printf("%s, perror %s.\n",
          openh_perror(OPENH_ERR_NOMEM),
          strerror(errno));
       return OPENH_ERR_NOMEM;
    }

    int i;
    for (i = 0; i < numaccelerators; i++)
    {
       CPU_ZERO(&accelerator_cpusets[i]);
    }

    int *logical_cpuids;
    int rc = openh_get_logical_cpuids(
                  &num_logical_cpuids_global,
                  &logical_cpuids);
    if (rc != OPENH_SUCCESS)
    {
       return rc;
    }

    free(logical_cpuids);

    int *physical_cpuids;
    rc = openh_get_physical_cpuids(
              &num_physical_cpuids_global,
              &physical_cpuids);
    if (rc != OPENH_SUCCESS)
    {
       return rc;
    }

    free(physical_cpuids);

    cpu_cpusets = (cpu_set_t*)malloc(
                  sizeof(cpu_set_t)*num_logical_cpuids_global);

    if (cpu_cpusets == NULL)
    {
       openh_err_printf("%s, perror %s.\n",
          openh_perror(OPENH_ERR_NOMEM),
          strerror(errno));
       return OPENH_ERR_NOMEM;
    }

    for (i = 0; i < num_logical_cpuids_global; i++)
    {
       CPU_ZERO(&cpu_cpusets[i]);
    }

    logical_cpu_pinned = (int*)calloc(
                         num_logical_cpuids_global, sizeof(int));
    physical_cpu_pinned = (int*)calloc(
                         num_physical_cpuids_global, sizeof(int));

    num_acc_components = 0;
    num_cpu_components = 0;

    /*
     * Build the logical to physical mapping of cpuids.
     */
    logical_to_physical_map = (int*)malloc(
                              sizeof(int)*num_logical_cpuids_global);
    for (i = 0; i < num_logical_cpuids_global; i++)
    {
        rc = openh_lcpuid_to_pcoreid(i, &logical_to_physical_map[i]);
        if (rc != OPENH_SUCCESS)
        {
           return rc;
        }
    }

    return OPENH_SUCCESS;
}

/*--------------------------------------------------------*/

int openh_finalize()
{
    free(accelerator_cpusets);

    free(cpu_cpusets);

    free(logical_cpu_pinned);

    return OPENH_SUCCESS;
}

/*--------------------------------------------------------*/

int openh_print()
{
    int i;

    printf("Master thread pinned to the CPUs: ");

    for (i = 0; i < num_logical_cpuids_global; i++)
    {
        if (CPU_ISSET(i, &main_cpuset))
        {
           if (!logical_cpu_pinned[i])
           {
              openh_err_printf(
                "CPU ID %d set in main cpuset ", 
                "but not in logical_cpu_pinned array: Internal error.", 
                i);
           }

           printf("%zu ", i);
        }
    }

    printf("\n");

    int cid = 0;
    for (cid = 0; cid < num_acc_components; cid++)
    {
        printf("Accelerator component %d pinned to the CPUs: ", cid);

        for (i = 0; i < num_logical_cpuids_global; i++)
        {
            if (CPU_ISSET(i, &accelerator_cpusets[cid]))
            {
               if (!logical_cpu_pinned[i])
               {
                  openh_err_printf("CPU ID %d set in accelerator cpuset %d", 
                       " but not in logical_cpu_pinned array: Internal error.", 
                       i, cid);
               }

               printf("%zu ", i);
            }
        }

        printf("\n");
    }

    for (cid = 0; cid < num_cpu_components; cid++)
    {
        printf("CPU component %d pinned to the CPUs: ", cid);

        for (i = 0; i < num_logical_cpuids_global; i++)
        {
            if (CPU_ISSET(i, &cpu_cpusets[cid]))
            {
               if (!logical_cpu_pinned[i])
               {
                  openh_err_printf("CPU ID %d set in cpu component cpuset %d", 
                       " but not in logical_cpu_pinned array: Internal error.\n", 
                       i, cid);
               }

               printf("%zu ", i);
            }
        }

        printf("\n");
    }

    return OPENH_SUCCESS;
}

/*--------------------------------------------------------*/

unsigned int openh_get_verbosity()
{
    return openh_verbosity;
}

/*--------------------------------------------------------*/

void openh_set_verbosity(
     unsigned int verbosity
)
{
    openh_verbosity = verbosity;
    return;
}

/*--------------------------------------------------------*/

// replaced by function "openh_lcpuid_to_pcoreid". 
int openh_siblings_list_first(int lcpuid, int* pcpuid) {
    FILE* fp;
    char* line = NULL;
    size_t len = 0;
    ssize_t read;

    char popenCmd[1024];
    sprintf(popenCmd, "%s%d%s",
        "cat /sys/devices/system/cpu/cpu",
        lcpuid,
        "/topology/thread_siblings_list");

    fp = popen(popenCmd, "r");
    if (fp == NULL) {
       openh_err_printf("%s %s, perror %s.",
          popenCmd,
          openh_perror(OPENH_ERR_POPEN),
          strerror(errno));
       return OPENH_ERR_POPEN;
    }

    /*
     * Just get the first CPU ID from the list.
     */
    while ((read = getline(&line, &len, fp)) != -1) {
       char* token = strtok(line, ",");
       while (token) {
          *pcpuid = atoi(token);
          goto LPCLOSE;
       }
    }

LPCLOSE:
    pclose(fp);
    if (line) {
       free(line);
    }

    return OPENH_SUCCESS;
}


// A function to get the physical core ID from the logical CPU ID regardless of the mapping scheme
int openh_lcpuid_to_pcoreid(int lcpuid, int* pcoreid) {
    hwloc_topology_t topology;
    hwloc_topology_init(&topology);
    hwloc_topology_load(topology);
    // Get the depth of the core object
    int core_depth = hwloc_get_type_depth(topology, HWLOC_OBJ_CORE);
    if (core_depth == HWLOC_TYPE_DEPTH_UNKNOWN) {
        hwloc_topology_destroy(topology);
        return -1;
    }
    // Iterate over all physical core objects and check if the logical CPU ID is in children
    hwloc_obj_t core = hwloc_get_obj_by_type(topology, HWLOC_OBJ_CORE, 0);
    while (core != NULL) {
        for (int j = 0; j < core->arity; j++) {
            if (core->children[j]->os_index == lcpuid) {
                *pcoreid = core->os_index;
                hwloc_topology_destroy(topology);
                return OPENH_SUCCESS;
            }
        }
        core = core->next_cousin;
    }
    hwloc_topology_destroy(topology);
    return -1;
}


/*--------------------------------------------------------*/

int areStringsEqual(const char* str1, const char* str2) {
    if (strcmp(str1, str2) == 0) {
        return 1; // Strings are equal
    } else {
        return 0; // Strings are not equal
    }
}

/*--------------------------------------------------------*/

int openh_get_num_lcores() {
	char type[128];
	hwloc_topology_t topology;
	hwloc_topology_init(&topology);
	hwloc_topology_load(topology);
	int topodepth = hwloc_topology_get_depth(topology);
	int num_logical_cpuids = 0;
	for (int depth = 0; depth < topodepth; depth++) {
        for (int i = 0; i < hwloc_get_nbobjs_by_depth(topology, depth); i++) {
            hwloc_obj_type_snprintf(type, sizeof(type), hwloc_get_obj_by_depth(topology, depth, i), 0);
            if (areStringsEqual(type, "PU")) {
				num_logical_cpuids++;
			}
      }
   }
   hwloc_topology_destroy(topology);
	return num_logical_cpuids;
}
/*--------------------------------------------------------*/

int openh_get_num_pcores() {
	char type[128];
	hwloc_topology_t topology;
	hwloc_topology_init(&topology);
	hwloc_topology_load(topology);
	int topodepth = hwloc_topology_get_depth(topology);
	int num_physical_cpuids = 0;
	for (int depth = 0; depth < topodepth; depth++) {
        for (int i = 0; i < hwloc_get_nbobjs_by_depth(topology, depth); i++) {
            hwloc_obj_type_snprintf(type, sizeof(type), hwloc_get_obj_by_depth(topology, depth, i), 0);
            if (areStringsEqual(type, "Core")) {
				num_physical_cpuids++;
			}
        }
    }
    hwloc_topology_destroy(topology);
	return num_physical_cpuids;
}

/*--------------------------------------------------------*/

// dependencies: get_num_logical_cpuids
int openh_get_logical_cpuids(int* num_logical_cpuids, int** logical_cpuids) {
	*num_logical_cpuids = openh_get_num_lcores();
	*logical_cpuids = malloc(*num_logical_cpuids * sizeof(int));
	char type[128];
	hwloc_topology_t topology;
	hwloc_topology_init(&topology);
	hwloc_topology_load(topology);
	int topodepth = hwloc_topology_get_depth(topology);
	int cnt = 0;
	for (int depth = 0; depth < topodepth; depth++) {
        for (int i = 0; i < hwloc_get_nbobjs_by_depth(topology, depth); i++) {
            hwloc_obj_type_snprintf(type, sizeof(type), hwloc_get_obj_by_depth(topology, depth, i), 0);
            if (areStringsEqual(type, "PU")) {
				(*logical_cpuids)[cnt++] = hwloc_get_obj_by_depth(topology, depth, i)->os_index;
			}
        }
    }
    hwloc_topology_destroy(topology);
    return OPENH_SUCCESS;
}


/*--------------------------------------------------------*/


// dependencies: get_num_physical_cpuids
int openh_get_physical_cpuids(int* num_physical_cpuids, int** physical_cpuids) {
	*num_physical_cpuids = openh_get_num_pcores();
	*physical_cpuids = malloc(*num_physical_cpuids * sizeof(int));
	char type[128];
	hwloc_topology_t topology;
	hwloc_topology_init(&topology);
	hwloc_topology_load(topology);
	int topodepth = hwloc_topology_get_depth(topology);
	int cnt = 0;
	for (int depth = 0; depth < topodepth; depth++) {
		for (int i = 0; i < hwloc_get_nbobjs_by_depth(topology, depth); i++) {
			hwloc_obj_type_snprintf(type, sizeof(type), hwloc_get_obj_by_depth(topology, depth, i), 0);
			if (areStringsEqual(type, "Core")) {
				(*physical_cpuids)[cnt++] = hwloc_get_obj_by_depth(topology, depth, i)->os_index;
			}
		}
	}
   hwloc_topology_destroy(topology);
   return OPENH_SUCCESS;
}

/*--------------------------------------------------------*/


int openh_is_hyperthreaded() {
    hwloc_topology_t topology;
    hwloc_topology_init(&topology);
	hwloc_topology_load(topology);
    hwloc_obj_t obj;
    int depth, num_cores = 0, num_pus = 0;
    depth = hwloc_topology_get_depth(topology);
    for (int i = 0; i < depth; ++i) {
        obj = NULL;
        while ((obj = hwloc_get_next_obj_by_depth(topology, i, obj)) != NULL) {
            if (obj->type == HWLOC_OBJ_CORE) {
                num_cores++;
            }   
            if (obj->type == HWLOC_OBJ_PU) {
                num_pus++;
            }
        }
    }
    hwloc_topology_destroy(topology);
    return num_cores < num_pus;
}


/*--------------------------------------------------------*/

int openh_get_mapping_scheme()
{
    return OPENH_L2P_ROUNDROBIN;
}

/*--------------------------------------------------------*/

int openh_roundrobin_pcoreid_func(int openh_lcoreid)
{
    int pcpuid;
    int rc = openh_lcpuid_to_pcoreid(openh_lcoreid, &pcpuid);
    if (rc != OPENH_SUCCESS)
    {
       return rc;
    }

    return pcpuid;   
}

/*--------------------------------------------------------*/

openh_pcoreid_func openh_get_mapping_function(
     int mapping_scheme)
{
    return openh_roundrobin_pcoreid_func;
}

/*--------------------------------------------------------*/

int openh_ptol_general_function(int pcoreid, int** lcpuids, int* nlids) {
    hwloc_topology_t topology;
	hwloc_topology_init(&topology);
	hwloc_topology_load(topology);
    hwloc_obj_t core = NULL;
    int num_pus = 0;

    // Find the specified core
    hwloc_obj_t obj = hwloc_get_obj_by_type(topology, HWLOC_OBJ_CORE, pcoreid);
    if (obj != NULL) {
        core = obj;
        for (int j = 0; j < core->arity; j++) {
            if (core->children[j]->type == HWLOC_OBJ_PU) {
                num_pus++;
            }
        }
    }

    // Set the number of logical IDs and allocate memory
    *nlids = num_pus;
    *lcpuids = malloc(num_pus * sizeof(int));

    // Populate the logical CPU IDs
    if (core != NULL) {
        for (int i = 0; i < core->arity; i++) {
            (*lcpuids)[i] = core->children[i]->logical_index;
        }
        hwloc_topology_destroy(topology);
        return 0;
    }
    hwloc_topology_destroy(topology);
    return -1;
}

/*--------------------------------------------------------*/

openh_ptol_func openh_get_ptol_function(
     int mapping_scheme)
{
    /*
     * Currently only ROUNDROBIN is implemented.  -> now it's independent from the mapping scheme
     */
    return openh_ptol_general_function;
}

/*--------------------------------------------------------*/

int openh_get_num_accelerators()
{
#ifdef _OPENACC
    return acc_get_num_devices(acc_device_nvidia);
#endif

#ifdef _OPENMP
    int num_devices;
#pragma omp parallel num_threads(1)
    { 
        num_devices = omp_get_num_devices();
    }
    return num_devices;
#endif
}

/*--------------------------------------------------------*/

int openh_get_cpuaffinity_nvidia(
    int accnum,
    int** closestCpuids,
    int* ncpus
)
{
    int num_cpuids = sysconf(_SC_NPROCESSORS_ONLN);

    *ncpus = 0;
    (*closestCpuids) = (int*)malloc(sizeof(int)*num_cpuids);
    if ((*closestCpuids) == NULL)
    {
       openh_err_printf("%s, perror %s.\n",
          openh_perror(OPENH_ERR_NOMEM),
          strerror(errno));
       return OPENH_ERR_NOMEM;
    }

    int numacc = openh_get_num_accelerators();

    FILE* fp;
    char* line = NULL;
    size_t len = 0;
    ssize_t read;

    char nvidia_smi_str[128];
    sprintf(
       nvidia_smi_str,
       "nvidia-smi topo -m | awk '(NR==%d)' | awk '{print $%d}' | tr -d '\n'",
       accnum + 2, numacc+2);

    fp = popen(nvidia_smi_str, "r");
    if (fp == NULL) {
       openh_err_printf("%s %s, perror %s.",
          nvidia_smi_str,
          openh_perror(OPENH_ERR_POPEN),
          strerror(errno));
       return OPENH_ERR_POPEN;
    }

    int numLinesRead = 0;
    while ((read = getline(&line, &len, fp)) != -1) {
       openh_printf(
           "Nvidia SMI string for GPU %d: %s.",
           accnum, line);

       int j;
       char *str1, *str2, *token, *subtoken;
       char *saveptr1, *saveptr2;

       numLinesRead++;
       str1 = line;
       for (j = 1; ; j++, str1 = NULL) 
       {
           token = strtok_r(str1, ",", &saveptr1);
           if (token == NULL) {
              break;
           }

           openh_printf(
              "Token1: %d, token %s", j, token);

           int range[2];
           int k = 0;
           for (str2 = token; ; str2 = NULL) {
               subtoken = strtok_r(str2, "-", &saveptr2);
               if (subtoken == NULL) {
                  break;
               }

               openh_printf(" --> %s\n", subtoken);

               range[k] = atoi(subtoken);
               k++;
           }
       
           if (k != 2)
           {
              openh_err_printf("%s %s.",
                 nvidia_smi_str,
                 openh_perror(OPENH_ERR_TOKEN));
              return OPENH_ERR_TOKEN;
           }

           for (k = range[0]; k <= range[1]; k++)
           {
               (*closestCpuids)[(*ncpus)] = k;
               (*ncpus)++;
           }
       }
    }

    if (numLinesRead > 1)
    {
       openh_err_printf("%s %s, perror %s.",
          "Too many lines in popen output",
          openh_perror(OPENH_ERR_POPEN),
          strerror(errno));
       return OPENH_ERR_POPEN;
    }

    pclose(fp);
    if (line) {
       free(line);
    }

    return OPENH_SUCCESS;
}

/*--------------------------------------------------------*/

int openh_get_accelerator_pcpuaffinity(
    int accnum,
    int** closestPcpuids,
    int* npcpus
)
{
    if (!openh_is_hyperthreaded())
    {
       return openh_get_cpuaffinity_nvidia(
               accnum, closestPcpuids, npcpus);
    }

    int* closestLcpuids;
    int nlcpus;

    int rc = openh_get_accelerator_lcpuaffinity(
                  accnum, &closestLcpuids, &nlcpus);
    if (rc != OPENH_SUCCESS)
    {
       return OPENH_SUCCESS;
    }

    (*npcpus) = 0;
    int* l2p = (int*)calloc(nlcpus, sizeof(int));

    int i;
    for (i = 0; i < nlcpus; i++)
    {
        int pcpuid;
        rc = openh_lcpuid_to_pcoreid(i, &pcpuid);
        if (rc != OPENH_SUCCESS)
        {
           return rc;
        }

        if (!l2p[pcpuid])
        {
           l2p[pcpuid] = 1;
           (*npcpus) += 1;
        }
    }

    for (i = 0; i < nlcpus; i++)
    {
        if (l2p[i])
        {
           (*closestPcpuids)[i] = i;
        }
    }

    free(l2p);

    return OPENH_SUCCESS;
}

/*--------------------------------------------------------*/

int openh_get_accelerator_lcpuaffinity(
    int accnum,
    int** closestCpuids,
    int* ncpus
)
{
    return openh_get_cpuaffinity_nvidia(
               accnum, closestCpuids, ncpus);
}

/*--------------------------------------------------------*/

int openh_get_acc_type(int id)
{
    return OPENH_CUDA_GPU;
}

/*--------------------------------------------------------*/

int openh_get_unique_pcore(int accId)
{
    int i, unique_pcore;
    for (i = 0; i < num_physical_cpuids_global; i++)
    {
        if (!physical_cpu_pinned[i])
        {
           unique_pcore = i;
        }
    }

    if (i == num_physical_cpuids_global)
    {
       openh_printf("All OpenH physical CPUs are pinned.");
       return -1;
    }

    return unique_pcore; 
}

/*--------------------------------------------------------*/

int openh_get_unique_lcore(int accId)
{
    int i, unique_lcore;

    for (i = 0; i < num_logical_cpuids_global; i++)
    {
        if (!logical_cpu_pinned[i])
        {
           /*
            * Check if its physical core is pinned.
            * If not return it.
            */
           if (!physical_cpu_pinned[logical_to_physical_map[i]])
           {
              unique_lcore = i;
           }
        }
    }

    if (i == num_logical_cpuids_global)
    {
       openh_printf("All OpenH physical or logical CPUs are pinned.");
       return -1;
    }

    return unique_lcore;
}

/*--------------------------------------------------------*/

void openh_assign_main_pcpuid(int cpuid)
{
     CPU_ZERO(&main_cpuset);
     CPU_SET(cpuid, &main_cpuset);
     physical_cpu_pinned[cpuid] = 1;
}

/*--------------------------------------------------------*/

void openh_assign_main_lcpuid(int cpuid)
{
     CPU_ZERO(&main_cpuset);
     CPU_SET(cpuid, &main_cpuset);
     logical_cpu_pinned[cpuid] = 1;
}

/*--------------------------------------------------------*/

void openh_assign_acc_pcpuids(
     int accComponentId,
     int *cpuids,
     int size)
{
     int i;

     CPU_ZERO(&accelerator_cpusets[accComponentId]);
 
     for (i = 0; i < size; i++)
     {
         if (physical_cpu_pinned[cpuids[i]])
         {
            openh_printf("OpenH physical CPU %d already set for pinning: May not result in "
                         "optimal performance.", cpuids[i]);
         }

         CPU_SET(cpuids[i], &accelerator_cpusets[accComponentId]);

         physical_cpu_pinned[cpuids[i]] = 1;
     }

     num_acc_components++;
}

/*--------------------------------------------------------*/

void openh_assign_acc_lcpuids(
     int accComponentId,
     int *lcpuids,
     int size)
{
     int i;

     CPU_ZERO(&accelerator_cpusets[accComponentId]);
 
     for (i = 0; i < size; i++)
     {
         if (logical_cpu_pinned[lcpuids[i]])
         {
            openh_printf("OpenH logical CPU %d already set for pinning: May not result in "
                         "optimal performance.", lcpuids[i]);
         }

         CPU_SET(lcpuids[i], &accelerator_cpusets[accComponentId]);

         logical_cpu_pinned[lcpuids[i]] = 1;
     }

     num_acc_components++;
}

/*--------------------------------------------------------*/

void openh_assign_cpu_lcpuids(
     int cpuComponentId, 
     int *cpuids, 
     int size)
{
     int i;

     CPU_ZERO(&cpu_cpusets[cpuComponentId]);
 
     for (i = 0; i < size; i++)
     {
         if (logical_cpu_pinned[cpuids[i]])
         {
            openh_printf("OpenH logical CPU %d already set for pinning: May not result in "
                         "optimal performance.", cpuids[i]);
         }

         CPU_SET(cpuids[i], &cpu_cpusets[cpuComponentId]);

         logical_cpu_pinned[cpuids[i]] = 1;
     }

     num_cpu_components++;
}

/*--------------------------------------------------------*/

void openh_assign_cpu_pcpuids(
     int cpuComponentId, 
     int *lcpuids, 
     int size)
{
     int i;

     CPU_ZERO(&cpu_cpusets[cpuComponentId]);
 
     for (i = 0; i < size; i++)
     {
         if (physical_cpu_pinned[lcpuids[i]])
         {
            openh_printf("OpenH physical CPU %d already set for pinning: May not result in "
                         "optimal performance.", lcpuids[i]);
         }

         CPU_SET(lcpuids[i], &cpu_cpusets[cpuComponentId]);

         physical_cpu_pinned[lcpuids[i]] = 1;
     }

     num_cpu_components++;
}

/*--------------------------------------------------------*/

void openh_assign_cpu_free_lcpuids(
     int cpuComponentId)
{
     int i;

     CPU_ZERO(&cpu_cpusets[cpuComponentId]);
 
     for (i = 0; i < num_logical_cpuids_global; i++)
     {
         if (!logical_cpu_pinned[i])
         {
            CPU_SET(i, &cpu_cpusets[cpuComponentId]);
            logical_cpu_pinned[i] = 1;
         }
     }

     num_cpu_components++;
}

/*--------------------------------------------------------*/

void openh_assign_cpu_free_pcpuids(
     int cpuComponentId)
{
     int i;

     CPU_ZERO(&cpu_cpusets[cpuComponentId]);
 
     for (i = 0; i < num_physical_cpuids_global; i++)
     {
         if (!physical_cpu_pinned[i])
         {
            CPU_SET(i, &cpu_cpusets[cpuComponentId]);
            physical_cpu_pinned[i] = 1;
         }
     }

     num_cpu_components++;
}

/*--------------------------------------------------------*/

int openh_bind_main_self()
{
    if (sched_setaffinity(0, sizeof(cpu_set_t), &main_cpuset) == -1) {
       openh_err_printf("%s, perror %s.\n",
          openh_perror(OPENH_ERR_PINMASTER),
          strerror(errno));
       return OPENH_ERR_PINMASTER;
    }

    return OPENH_SUCCESS;
}

/*--------------------------------------------------------*/

int openh_bind_acc_self(int accId)
{
    if (sched_setaffinity(0, sizeof(cpu_set_t), 
             &accelerator_cpusets[accId]) == -1) {
       openh_err_printf("%s %d, perror %s.\n",
          openh_perror(OPENH_ERR_PINACCCID),
          accId,
          strerror(errno));
       return OPENH_ERR_PINACCCID;
    }

    return OPENH_SUCCESS;
}

/*--------------------------------------------------------*/

int openh_bind_cpu_self(int cpuComponentId)
{
    if (sched_setaffinity(0, sizeof(cpu_set_t),
             &cpu_cpusets[cpuComponentId]) == -1) {
       openh_err_printf("%s %d, perror %s.\n",
          openh_perror(OPENH_ERR_PINCPUCID),
          cpuComponentId,
          strerror(errno));
       return OPENH_ERR_PINCPUCID;
    }

    return OPENH_SUCCESS;
}

/*--------------------------------------------------------*/