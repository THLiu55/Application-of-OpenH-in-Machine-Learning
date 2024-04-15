
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

    openh_finalize();

    exit(EXIT_SUCCESS);
}

/*--------------------------------------------------------*/


/*

Number of accelerators: 2

*/