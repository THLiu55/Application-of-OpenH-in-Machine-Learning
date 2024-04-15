
/*--------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <openh.h>
#include <openherr.h>

/*--------------------------------------------------------*/

int main(int argc, char** argv)
{
    int num_physical_cpuids;
    int *physical_cpuids;
    int i, rc;

    openh_init();

    rc = openh_get_physical_cpuids(
             &num_physical_cpuids,
             &physical_cpuids);

    if (rc != OPENH_SUCCESS)
    {
       exit(EXIT_FAILURE);
    }

    printf("Physical CPU ids: ");

    for (i = 0; i < num_physical_cpuids; i++)
    {
        printf("%zu ", physical_cpuids[i]);
    }

    printf("\n");

    free(physical_cpuids);

    openh_finalize();

    exit(EXIT_SUCCESS);
}

/*--------------------------------------------------------*/

/*

Physical CPU ids: 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31

*/