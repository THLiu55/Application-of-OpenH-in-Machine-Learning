
/*--------------------------------------------------------*/

#include <stdlib.h>
#include <openh.h>

/*--------------------------------------------------------*/

int main(int argc, char** argv)
{
    openh_init();

    openh_assign_main_pcpuid(0);
    openh_bind_main_self();

    openh_print();

    openh_finalize();

    exit(EXIT_SUCCESS);
}

/*--------------------------------------------------------*/

/*

(OPENH HYBRID APP MODEL) ==> (ERROR) ==> openh_print:175 ==> CPU ID -1969253310 set in main cpuset 
Master thread pinned to the CPUs: 0 

*/