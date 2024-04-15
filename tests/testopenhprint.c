/*--------------------------------------------------------*/

#include <stdlib.h>
#include <openh.h>

/*--------------------------------------------------------*/

int main(int argc, char** argv)
{
    openh_init();
    openh_print();
    openh_finalize();
    exit(EXIT_SUCCESS);
}

/*--------------------------------------------------------*/
/*

Master thread pinned to the CPUs:

*/