
/*--------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <openh.h>
#include <openherr.h>

/*--------------------------------------------------------*/

int main(int argc, char** argv)
{
    int num_logical_cpuids;
    int *logical_cpuids;
    int i, rc;

    openh_init();

    rc = openh_get_logical_cpuids(
             &num_logical_cpuids,
             &logical_cpuids);

    if (rc != OPENH_SUCCESS)
    {
       exit(EXIT_FAILURE);
    }

    printf("Logical CPU ids: ");

    for (i = 0; i < num_logical_cpuids; i++)
    {
        printf("%zu ", logical_cpuids[i]);
    }

    printf("\n");

    free(logical_cpuids);

    openh_finalize();

    exit(EXIT_SUCCESS);
}

/*--------------------------------------------------------*/

/*

Logical CPU ids: 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59 60 61 62 63

*/