
/*--------------------------------------------------------*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <omp.h>

/*--------------------------------------------------------*/

int main(int argc, char** argv)
{
    int numProcessors = omp_get_num_procs();
    printf("Number of cores %d.\n", numProcessors);

#pragma omp parallel num_threads(2) proc_bind(spread)
    {
        int pnum = omp_get_thread_num();
        printf("My thread num %d.\n", pnum);
        if (pnum == 0)
#pragma omp parallel num_threads(1) proc_bind(spread)
        {
           int lnum = omp_get_thread_num();
           int cpunum = sched_getcpu();
           printf("My parent thread num %d, level thread num %d, cpu num %d.\n", 
		  pnum, lnum, cpunum);
	}

        if (pnum == 1)
#pragma omp parallel num_threads(63) proc_bind(close)
        {
           int lnum = omp_get_thread_num();
           int cpunum = sched_getcpu();
           printf("My parent thread num %d, level thread num %d, cpu num %d.\n", 
		  pnum, lnum, cpunum);
	}
    }

    exit(EXIT_SUCCESS);
}

/*--------------------------------------------------------*/

/*

Number of cores 64.
My thread num 1.
My parent thread num 1, level thread num 0, cpu num 54.
My thread num 0.
My parent thread num 0, level thread num 0, cpu num 53.

*/