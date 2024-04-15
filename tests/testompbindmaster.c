
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

#pragma omp parallel num_threads(numProcessors) proc_bind(master)
    {
        int pnum = omp_get_thread_num();
        int cpunum = sched_getcpu();
        printf("My thread num %d, cpu num %d.\n", pnum, cpunum);
    }

    exit(EXIT_SUCCESS);
}

/*--------------------------------------------------------*/


/*

Number of cores 64.
My thread num 0, cpu num 53.
My thread num 30, cpu num 51.
My thread num 51, cpu num 9.
My thread num 23, cpu num 44.
My thread num 36, cpu num 58.
My thread num 26, cpu num 15.
My thread num 55, cpu num 45.
My thread num 20, cpu num 41.
My thread num 48, cpu num 38.
My thread num 17, cpu num 6.
My thread num 35, cpu num 25.
My thread num 53, cpu num 11.
My thread num 22, cpu num 43.
My thread num 19, cpu num 40.
My thread num 8, cpu num 61.
My thread num 50, cpu num 8.
My thread num 1, cpu num 54.
My thread num 4, cpu num 57.
My thread num 44, cpu num 34.
My thread num 6, cpu num 27.
My thread num 54, cpu num 12.
My thread num 37, cpu num 59.
My thread num 29, cpu num 18.
My thread num 61, cpu num 19.
My thread num 33, cpu num 23.
My thread num 3, cpu num 56.
My thread num 34, cpu num 24.
My thread num 59, cpu num 49.
My thread num 28, cpu num 17.
My thread num 58, cpu num 16.
My thread num 27, cpu num 48.
My thread num 49, cpu num 7.
My thread num 18, cpu num 39.
My thread num 16, cpu num 37.
My thread num 63, cpu num 21.
My thread num 2, cpu num 55.
My thread num 56, cpu num 46.
My thread num 25, cpu num 14.
My thread num 62, cpu num 52.
My thread num 31, cpu num 20.
My thread num 41, cpu num 31.
My thread num 42, cpu num 32.
My thread num 11, cpu num 0.
My thread num 15, cpu num 36.
My thread num 10, cpu num 63.
My thread num 47, cpu num 5.
My thread num 9, cpu num 62.
My thread num 5, cpu num 26.
My thread num 32, cpu num 22.
My thread num 7, cpu num 60.
My thread num 38, cpu num 28.
My thread num 40, cpu num 30.
My thread num 60, cpu num 50.
My thread num 13, cpu num 2.
My thread num 14, cpu num 35.
My thread num 45, cpu num 3.
My thread num 43, cpu num 33.
My thread num 12, cpu num 1.
My thread num 39, cpu num 29.
My thread num 57, cpu num 47.
My thread num 24, cpu num 13.
My thread num 46, cpu num 4.
My thread num 52, cpu num 10.
My thread num 21, cpu num 42.

*/