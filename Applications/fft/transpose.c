
/*----------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <omp.h>

/*----------------------------------------------------------------*/

#include <fftw3.h>

/*----------------------------------------------------------------*/

#define min(a,b) ((a < b) ? a : b)

/*----------------------------------------------------------------*/

int hcl_fillSignal2D(
    const int m,
    const int n,
    const int start,
    const unsigned int nt,
    fftw_complex* signal
)
{
    printf("Initializing the signal\n");

    time_t t;
    srand((unsigned) time(&t));

    int p, q;
#pragma omp parallel for shared(signal,start) num_threads(nt)
    for (p = start; p < m; p++)
    {
        for (q = 0; q < n; q++)
        {
            int index = p*n+q;
            signal[index][0] = rand() % (index+1);
            signal[index][1] = rand() % (index+1);
        }
    }
 
    printf("Initialized the signal\n");

    return 0;
}

/*----------------------------------------------------------------*/

int hcl_fillSignal2D_nonrand(
    const int m,
    const int n,
    const int start,
    const unsigned int nt,
    fftw_complex* signal
)
{
    printf("Initializing the signal\n");

    int p, q;
#pragma omp parallel for shared(signal,start) num_threads(nt)
    for (p = start; p < m; p++)
    {
        for (q = 0; q < n; q++)
        {
            int index = p*n+q;
            signal[index][0] = (index+1);
            signal[index][1] = (index+1);
        }
    }
 
    printf("Initialized the signal\n");

    return 0;
}

/*----------------------------------------------------------------*/

int hcl_checkTranspose(
    const int m,
    const int n,
    const unsigned int nt,
    const unsigned int verbosity,
    fftw_complex* s1,
    fftw_complex* s2
)
{
    int p, q;
    for (p = 0; p < m; p++)
    {
        for (q = 0; q < n; q++)
        {
            printf(
              "s1(%lf,%lf), s2(%lf,%lf). ",
              s1[p*n+q][0], s1[p*n+q][1],
              s2[p*n+q][0], s2[p*n+q][1]
            );
        }
        if (verbosity)
        {
           printf("\n");
        }
    }
 
    return 1;
}

/*----------------------------------------------------------------*/

void hcl_transpose(
    const int n,
    const unsigned int nt,
    fftw_complex* X
)
{
    int p, q;

#pragma omp parallel for shared(X) private(p, q) num_threads(nt)
    for (p = 0; p < n; p++)
    {
        for (q = p+1; q < n; q++)
        {
           double tmpr = X[p*n+q][0];
           double tmpi = X[p*n+q][1];
           X[p*n+q][0] = X[q*n+p][0];
           X[p*n+q][1] = X[q*n+p][1];
           X[q*n+p][0] = tmpr;
           X[q*n+p][1] = tmpi;
        }
    }
    return;
}

/*----------------------------------------------------------------*/

void hcl_transpose_scalar_block(
    fftw_complex* X1,
    fftw_complex* X2,
    const int i, const int j,
    const int n,
    const int block_size,
    const unsigned int verbosity) 
{
    int p, q;

    for (p = 0; p < min(n-i,block_size); p++) {
        for (q = 0; q < min(n-j,block_size); q++) {

           int index1 = i*n+j + p*n+q;
           int index2 = j*n+i + q*n+p;

           if (index1 >= index2)
              continue;

           if (verbosity)
              printf(
                  "%d: i %d, j %d, p %d, q %d, index1 %d index2 %d\n", 
                  omp_get_thread_num(), 
                  i, j,
                  p, q,
                  i*n+j + p*n+q, j*n+i + q*n+p);

           double tmpr = X1[p*n+q][0];
           double tmpi = X1[p*n+q][1];
           X1[p*n+q][0] = X2[q*n+p][0];
           X1[p*n+q][1] = X2[q*n+p][1];
           X2[q*n+p][0] = tmpr;
           X2[q*n+p][1] = tmpi;
        }
    }
}

void hcl_transpose_block(
    fftw_complex* X,
    const int start, const int end,
    const int n,
    const unsigned int nt,
    const int block_size,
    const unsigned int verbosity)
{
    int i, j;

#pragma omp parallel for shared(X) private(i, j) num_threads(nt)
    for (i = 0; i < end; i += block_size) {
        for (j = 0; j < end; j += block_size) {
            if (verbosity)
               printf(
                  "%d: i %d, j %d\n", 
                  omp_get_thread_num(), i, j);

            hcl_transpose_scalar_block(
                &X[start + i*n + j], 
                &X[start + j*n + i], i, j, n, block_size, verbosity);
        }
    }
}

/*----------------------------------------------------------------*/

void hcl_transpose_scalar_block_padded(
    fftw_complex* X1,
    fftw_complex* X2,
    const int i, const int j,
    const int n,
    const int npadded,
    const int block_size,
    const unsigned int verbosity) 
{
    int p, q;

    for (p = 0; p < min(n-i,block_size); p++) {
        for (q = 0; q < min(n-j,block_size); q++) {

           if (verbosity)
              printf(
                  "%d: i %d, j %d, p %d, q %d, index1 %d index2 %d\n", 
                  omp_get_thread_num(), 
                  i, j,
                  p, q,
                  i*npadded+j + p*npadded+q, j*npadded+i + q*npadded+p);

           double tmpr = X1[p*npadded+q][0];
           double tmpi = X1[p*npadded+q][1];
           X1[p*npadded+q][0] = X2[q*npadded+p][0];
           X1[p*npadded+q][1] = X2[q*npadded+p][1];
           X2[q*npadded+p][0] = tmpr;
           X2[q*npadded+p][1] = tmpi;
        }
    }
}

void hcl_transpose_block_padded(
    fftw_complex* X,
    const int start, const int end,
    const int n,
    const int npadded,
    const unsigned int nt,
    const int block_size,
    const unsigned int verbosity)
{
    int i, j;

#pragma omp parallel for shared(X) private(i, j) num_threads(nt)
    for (i = 0; i < end; i += block_size) {
        for (j = 0; j < end; j += block_size) {
            if (verbosity)
               printf(
                  "%d: i %d, j %d\n", 
                  omp_get_thread_num(), i, j);

            hcl_transpose_scalar_block_padded(
                &X[start + i*npadded + j], 
                &X[start + j*npadded + i], i, j, n, npadded, 
                block_size, verbosity);
        }
    }
}

/*----------------------------------------------------------------*/
