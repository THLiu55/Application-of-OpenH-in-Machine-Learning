
/*--------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <openh.h>
#include <openherr.h>

/*--------------------------------------------------------*/

int main(int argc, char** argv)
{
    openh_init();

    int numaccelerators = openh_get_num_accelerators();

    printf("Number of accelerators: %d\n", numaccelerators);

    int i;
    for (i = 0; i < numaccelerators; i++)
    {
        int* closestCpuids, ncpus;
        int rc = openh_get_accelerator_lcpuaffinity(
                      i, &closestCpuids, &ncpus);

        if (rc != OPENH_SUCCESS)
        {
           exit(EXIT_SUCCESS);
        }

        int c;
        printf("Accelerator %d: CPU affinity: ", i);
        for (c = 0; c < ncpus; c++)
        {
            printf("%zu ", closestCpuids[c]);
        }
        printf("\n");

        free(closestCpuids);
    }

    openh_finalize();

    exit(EXIT_SUCCESS);
}

/*--------------------------------------------------------*/


/*

Number of accelerators: 2
Accelerator 0: CPU affinity: 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59 60 61 62 63 
Accelerator 1: CPU affinity: 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59 60 61 62 63 

*/
