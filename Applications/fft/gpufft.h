
/*----------------------------------------------------------------*/

#ifndef _GPUTRANSPOSE_HH
#define _GPUTRANSPOSE_HH

/*-----------------------------------------------------------*/

#include <cuda_runtime.h>
#include <cuComplex.h>
#include <cufft.h>

/*----------------------------------------------------------------*/

int gpufftw1ds(
    const int sign, const int m, const int n,
    cufftDoubleComplex* X,
    int* registerMemory
);

int gpufftw1dns(
    const int sign, const int m, const int n,
    cufftDoubleComplex* X
);

/*----------------------------------------------------------------*/

#endif /* _GPUTRANSPOSE_HH */

/*----------------------------------------------------------------*/
