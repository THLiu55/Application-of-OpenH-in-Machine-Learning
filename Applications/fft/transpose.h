
/*----------------------------------------------------------------*/

#ifndef _TRANSPOSE_HH
#define _TRANSPOSE_HH

/*----------------------------------------------------------------*/

#include <fftw3.h>
#include <fftw3_mkl.h>

/*----------------------------------------------------------------*/

#define min(a,b) ((a < b) ? a : b)

/*----------------------------------------------------------------*/

int hcl_fillSignal2D(
    const int m,
    const int n,
    const int start,
    const unsigned int nt,
    fftw_complex* signal
);

/*----------------------------------------------------------------*/

int hcl_fillSignal2D_nonrand(
    const int m,
    const int n,
    const int start,
    const unsigned int nt,
    fftw_complex* signal
);

/*----------------------------------------------------------------*/

int hcl_checkTranspose(
    const int m,
    const int n,
    const unsigned int nt,
    const unsigned int verbosity,
    fftw_complex* s1,
    fftw_complex* s2
);

/*----------------------------------------------------------------*/

void hcl_transpose(
    const int n,
    const unsigned int nt,
    fftw_complex* X
);

/*----------------------------------------------------------------*/

void hcl_transpose_scalar_block(
    fftw_complex* X1,
    fftw_complex* X2,
    const int i,
    const int j,
    const int n,
    const int block_size,
    const unsigned int verbosity);

void hcl_transpose_block(
    fftw_complex* X,
    const int start, const int end,
    const int n,
    const unsigned int nt,
    const int block_size,
    const unsigned int verbosity);

/*----------------------------------------------------------------*/

void hcl_transpose_scalar_block_padded(
    fftw_complex* X1,
    fftw_complex* X2,
    const int i,
    const int j,
    const int n,
    const int npadded,
    const int block_size,
    const unsigned int verbosity);

void hcl_transpose_block_padded(
    fftw_complex* X,
    const int start, const int end,
    const int n,
    const int npadded,
    const unsigned int nt,
    const int block_size,
    const unsigned int verbosity);

/*----------------------------------------------------------------*/

#endif /* _TRANSPOSE_HH */

/*----------------------------------------------------------------*/

