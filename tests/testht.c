
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

    int ht = openh_is_hyperthreaded();

    if (ht)
    {
       printf("Platform is hyperthreading enabled.\n");
    }
    else
    {
       printf("Platform is not hyperthreading enabled.\n");
    }

    openh_print();

    openh_finalize();

    exit(EXIT_SUCCESS);
}

/*--------------------------------------------------------*/


/*

Platform is hyperthreading enabled.

*/
