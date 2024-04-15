
/*--------------------------------------------------------*/

#include <stdlib.h>
#include <openh.h>

/*--------------------------------------------------------*/

int main(int argc, char** argv)
{
    openh_init();

    int i;
    int nacc = openh_get_num_accelerators();

    int* closestCpuIds = (int*)malloc(sizeof(int)*nacc);
    for (i = 0; i < nacc; i++) {
        closestCpuIds[i] = i;
    }

    for (i = 0; i < nacc; i++) {
        openh_assign_acc_pcpuids(i, &closestCpuIds[i], 1);
    }

    free(closestCpuIds);
 
    openh_assign_cpu_free_pcpuids(0);

    openh_print();

    openh_finalize();

    exit(EXIT_SUCCESS);
}

/*--------------------------------------------------------*/

/*
 
 Master thread pinned to the CPUs: 
(OPENH HYBRID APP MODEL) ==> (ERROR) ==> openh_print:197 ==> CPU ID 2043634610 set in accelerator cpuset 0
Accelerator component 0 pinned to the CPUs: 0 
(OPENH HYBRID APP MODEL) ==> (ERROR) ==> openh_print:197 ==> CPU ID 2043634610 set in accelerator cpuset 1
Accelerator component 1 pinned to the CPUs: 1 
(OPENH HYBRID APP MODEL) ==> (ERROR) ==> openh_print:219 ==> CPU ID 2043634476 set in cpu component cpuset 2
(OPENH HYBRID APP MODEL) ==> (ERROR) ==> openh_print:219 ==> CPU ID 2043634476 set in cpu component cpuset 3
(OPENH HYBRID APP MODEL) ==> (ERROR) ==> openh_print:219 ==> CPU ID 2043634476 set in cpu component cpuset 4
(OPENH HYBRID APP MODEL) ==> (ERROR) ==> openh_print:219 ==> CPU ID 2043634476 set in cpu component cpuset 5
(OPENH HYBRID APP MODEL) ==> (ERROR) ==> openh_print:219 ==> CPU ID 2043634476 set in cpu component cpuset 6
(OPENH HYBRID APP MODEL) ==> (ERROR) ==> openh_print:219 ==> CPU ID 2043634476 set in cpu component cpuset 7
(OPENH HYBRID APP MODEL) ==> (ERROR) ==> openh_print:219 ==> CPU ID 2043634476 set in cpu component cpuset 8
(OPENH HYBRID APP MODEL) ==> (ERROR) ==> openh_print:219 ==> CPU ID 2043634476 set in cpu component cpuset 9
(OPENH HYBRID APP MODEL) ==> (ERROR) ==> openh_print:219 ==> CPU ID 2043634476 set in cpu component cpuset 10
(OPENH HYBRID APP MODEL) ==> (ERROR) ==> openh_print:219 ==> CPU ID 2043634476 set in cpu component cpuset 11
(OPENH HYBRID APP MODEL) ==> (ERROR) ==> openh_print:219 ==> CPU ID 2043634476 set in cpu component cpuset 12
(OPENH HYBRID APP MODEL) ==> (ERROR) ==> openh_print:219 ==> CPU ID 2043634476 set in cpu component cpuset 13
(OPENH HYBRID APP MODEL) ==> (ERROR) ==> openh_print:219 ==> CPU ID 2043634476 set in cpu component cpuset 14
(OPENH HYBRID APP MODEL) ==> (ERROR) ==> openh_print:219 ==> CPU ID 2043634476 set in cpu component cpuset 15
(OPENH HYBRID APP MODEL) ==> (ERROR) ==> openh_print:219 ==> CPU ID 2043634476 set in cpu component cpuset 16
(OPENH HYBRID APP MODEL) ==> (ERROR) ==> openh_print:219 ==> CPU ID 2043634476 set in cpu component cpuset 17
(OPENH HYBRID APP MODEL) ==> (ERROR) ==> openh_print:219 ==> CPU ID 2043634476 set in cpu component cpuset 18
(OPENH HYBRID APP MODEL) ==> (ERROR) ==> openh_print:219 ==> CPU ID 2043634476 set in cpu component cpuset 19
(OPENH HYBRID APP MODEL) ==> (ERROR) ==> openh_print:219 ==> CPU ID 2043634476 set in cpu component cpuset 20
(OPENH HYBRID APP MODEL) ==> (ERROR) ==> openh_print:219 ==> CPU ID 2043634476 set in cpu component cpuset 21
(OPENH HYBRID APP MODEL) ==> (ERROR) ==> openh_print:219 ==> CPU ID 2043634476 set in cpu component cpuset 22
(OPENH HYBRID APP MODEL) ==> (ERROR) ==> openh_print:219 ==> CPU ID 2043634476 set in cpu component cpuset 23
(OPENH HYBRID APP MODEL) ==> (ERROR) ==> openh_print:219 ==> CPU ID 2043634476 set in cpu component cpuset 24
(OPENH HYBRID APP MODEL) ==> (ERROR) ==> openh_print:219 ==> CPU ID 2043634476 set in cpu component cpuset 25
(OPENH HYBRID APP MODEL) ==> (ERROR) ==> openh_print:219 ==> CPU ID 2043634476 set in cpu component cpuset 26
(OPENH HYBRID APP MODEL) ==> (ERROR) ==> openh_print:219 ==> CPU ID 2043634476 set in cpu component cpuset 27
(OPENH HYBRID APP MODEL) ==> (ERROR) ==> openh_print:219 ==> CPU ID 2043634476 set in cpu component cpuset 28
(OPENH HYBRID APP MODEL) ==> (ERROR) ==> openh_print:219 ==> CPU ID 2043634476 set in cpu component cpuset 29
(OPENH HYBRID APP MODEL) ==> (ERROR) ==> openh_print:219 ==> CPU ID 2043634476 set in cpu component cpuset 30
(OPENH HYBRID APP MODEL) ==> (ERROR) ==> openh_print:219 ==> CPU ID 2043634476 set in cpu component cpuset 31
CPU component 0 pinned to the CPUs: 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31

*/
