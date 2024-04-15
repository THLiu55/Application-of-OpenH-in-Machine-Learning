
/*--------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <openh.h>

/*--------------------------------------------------------*/

int main(int argc, char** argv)
{
    if (argc != 2)
    {
       fprintf(stderr, "Usage: %s <verbosity>\n", argv[0]);
       exit(EXIT_FAILURE);
    }

    int verbosity = atoi(argv[1]);

    openh_init();

    openh_set_verbosity(verbosity);

    int npcores = openh_get_num_pcores();

    printf("Number of physical cores %d.\n", npcores);

    openh_print();

    openh_finalize();

    exit(EXIT_SUCCESS);
}

/*--------------------------------------------------------*/

/*

Number of logical cores 32.

*/