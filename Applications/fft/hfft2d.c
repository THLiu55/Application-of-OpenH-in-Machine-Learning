/*-----------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <omp.h>
#include <unistd.h>

/*-----------------------------------------------------------*/

#include "transpose.h"
#include "cpufft.h"
#include "gpufft.h"

/*-----------------------------------------------------------*/

int
fftw2d(
    const int sign, const unsigned int cpum,
    const unsigned int gpu1m, const unsigned int gpu2m,
    const int streaming,
    const int N, const unsigned int nt,
    const unsigned int blockSize,
    const unsigned int verbosity,
    fftw_complex* X,
    int* register1, int* register2
)
{
    int gpu1start = cpum*N;
    int gpu2start = cpum*N + gpu1m*N;

    printf(
       "(CPU,GPU1,GPU2) share of N: (%d,%d,%d).\n",
       cpum, gpu1m, gpu2m);

#pragma omp parallel sections num_threads(3)
{   
    #pragma omp section
    {
        if (cpum != 0)
        {
           printf("CPU FFT 1Ds...%d\n", cpum);
           cpufftw1d(sign, cpum, N, X, X);
        }
        else
        {
           printf("CPU FFT 1Ds...None to execute\n");
        }
    }
 
    #pragma omp section
    {
        if (gpu1m != 0)
        {
           printf("GPU1 FFT 1Ds...%d\n", gpu1m);

           cudaError_t err = cudaSetDevice(0);
           if (err != cudaSuccess) {
              printf("CUDA error %s on line %d in file %s\n", cudaGetErrorName(err), __LINE__, __FILE__);
              cudaDeviceReset();
           }

           if (streaming) {
              gpufftw1ds(
                 (sign == FFTW_FORWARD) ? CUFFT_FORWARD : CUFFT_INVERSE,
                 gpu1m, N,
                 (cufftDoubleComplex*)&X[gpu1start], register1);
           }
           else 
           {
              gpufftw1dns(
                 (sign == FFTW_FORWARD) ? CUFFT_FORWARD : CUFFT_INVERSE,
                 gpu1m, N,
                 (cufftDoubleComplex*)&X[gpu1start]);
           }
        }
        else
        {
           printf("GPU1 FFT 1Ds... None to execute\n");
        }
    }

    #pragma omp section
    {
        if (gpu2m != 0)
        {
           printf("GPU2 FFT 1Ds...%d\n", gpu2m);

           cudaError_t err = cudaSetDevice(1);
           if (err != cudaSuccess) {
              printf("CUDA error %s on line %d in file %s\n", cudaGetErrorName(err), __LINE__, __FILE__);
              cudaDeviceReset();
           }

           if (streaming) {
              gpufftw1ds(
                 (sign == FFTW_FORWARD) ? CUFFT_FORWARD : CUFFT_INVERSE,
                 gpu2m, N, 
                 (cufftDoubleComplex*)&X[gpu2start], register2);
           }
           else
           {
              gpufftw1dns(
                 (sign == FFTW_FORWARD) ? CUFFT_FORWARD : CUFFT_INVERSE,
                 gpu2m, N, 
                 (cufftDoubleComplex*)&X[gpu2start]);
           }
        }
        else
        {
           printf("GPU2 FFT 1Ds... None to execute\n");
        }
    }
}

    printf("Blocked transpose...block size %d\n", blockSize);
    hcl_transpose_block(
       X, 0, N, N,
       nt, blockSize, 0);

#pragma omp parallel sections num_threads(3)
{   
    #pragma omp section
    {
        if (cpum != 0)
        {
           printf("CPU FFT 1Ds...%d\n", cpum);
           cpufftw1d(sign, cpum, N, X, X);
        }
        else
        {
           printf("CPU FFT 1Ds...None to execute\n");
        }
    }
 
    #pragma omp section
    {
        if (gpu1m != 0)
        {
           printf("GPU1 FFT 1Ds...%d\n", gpu1m);

           cudaError_t err = cudaSetDevice(0);
           if (err != cudaSuccess) {
              printf("CUDA error %s on line %d in file %s\n", cudaGetErrorName(err), __LINE__, __FILE__);
              cudaDeviceReset();
           }

           if (streaming) {
              gpufftw1ds(
                 (sign == FFTW_FORWARD) ? CUFFT_FORWARD : CUFFT_INVERSE,
                 gpu1m, N,
                 (cufftDoubleComplex*)&X[gpu1start], register1);
           }
           else 
           {
              gpufftw1dns(
                 (sign == FFTW_FORWARD) ? CUFFT_FORWARD : CUFFT_INVERSE,
                 gpu1m, N,
                 (cufftDoubleComplex*)&X[gpu1start]);
           }
        }
        else
        {
           printf("GPU1 FFT 1Ds... None to execute\n");
        }
    }

    #pragma omp section
    {
        if (gpu2m != 0)
        {
           printf("GPU2 FFT 1Ds...%d\n", gpu2m);

           cudaError_t err = cudaSetDevice(1);
           if (err != cudaSuccess) {
              printf("CUDA error %s on line %d in file %s\n", cudaGetErrorName(err), __LINE__, __FILE__);
              cudaDeviceReset();
           }

           if (streaming) {
              gpufftw1ds(
                 (sign == FFTW_FORWARD) ? CUFFT_FORWARD : CUFFT_INVERSE,
                 gpu2m, N, 
                 (cufftDoubleComplex*)&X[gpu2start], register2);
           }
           else
           {
              gpufftw1dns(
                 (sign == FFTW_FORWARD) ? CUFFT_FORWARD : CUFFT_INVERSE,
                 gpu2m, N, 
                 (cufftDoubleComplex*)&X[gpu2start]);
           }
        }
        else
        {
           printf("GPU2 FFT 1Ds... None to execute\n");
        }
    }
}

    printf("Blocked transpose...block size %d\n", blockSize);
    hcl_transpose_block(
       X, 0, N, N,
       nt, blockSize, 0);

    return 0;
}

/*-----------------------------------------------------------*/

int main(int argc, char ** argv)
{
    if (argc != 8)
    {
       fprintf(
          stderr,
          "Usage: %s <N> <BlockSize> <NCPU> <NGPU1> <NGPU2> <GPU Streaming> <verbosity>\n"
          "Blocksize is the block size for transpose.\n"
          "<NCPU> + <NGPU1> + <NGPU2> must sum to 1.0.\n",
          argv[0]);
       exit(EXIT_FAILURE);
    }

    int N = atoi(argv[1]);
    int blockSize = atoi(argv[2]);
    double cpu = atof(argv[3]);
    double gpu1 = atof(argv[4]);
    double gpu2 = atof(argv[5]);
    int streaming = atoi(argv[6]);
    int verbosity = atoi(argv[7]);

    if (((cpu + gpu1 + gpu2) < 1.0)
        || ((cpu + gpu1 + gpu2) > 1.0)
    )
    {
       fprintf(
          stderr,
          "<NCPU> + <NGPU1> + <NGPU2> must sum to 1.0.\n"
       );
       exit(EXIT_FAILURE);
    }

    int cpum = cpu*N;
    int gpu1m = gpu1*N;
    int gpu2m = N - cpum - gpu1m;

    fftw_complex *X = (fftw_complex*)fftw_malloc(
                           N*N*sizeof(fftw_complex));
    if (X == NULL)
    {
       fprintf(
         stderr,
         "Memory allocation failure.\n"
       );
       exit(EXIT_FAILURE);
    }

    int ncores = sysconf(_SC_NPROCESSORS_ONLN);
    omp_set_max_active_levels(2);
    omp_set_dynamic(0);

    fftw_init_threads();
    fftw_plan_with_nthreads(ncores-2);

    hcl_fillSignal2D_nonrand(
       N, N, 0, ncores, X);

    struct timeval start, end;
    gettimeofday(&start, NULL);

    printf("HFFT FORWARD transform...\n");
    int register1 = 0;
    int register2 = 0;
    fftw2d(FFTW_FORWARD, cpum, gpu1m, gpu2m, streaming,
           N, ncores, blockSize, verbosity, X,
           &register1, &register2);

    printf("HFFT BACKWARD transform...\n");
    fftw2d(FFTW_BACKWARD, cpum, gpu1m, gpu2m, streaming,
           N, ncores, blockSize, verbosity, X,
           &register1, &register2);

    gettimeofday(&end, NULL);

    fftw_cleanup_threads();
    fftw_free(X);

    double tstart = start.tv_sec + start.tv_usec/1000000.;
    double tend = end.tv_sec + end.tv_usec/1000000.;
    double hfft = (tend - tstart);
    double pSize = (double)N*(double)N;
    double speed = 2.5 * pSize * log2(pSize) / hfft;
    double mflops = speed * 1e-06;

    printf(
        "n=%d, time(sec)=%0.6f, speed(mflops)=%0.6f\n",
        N, hfft, mflops
    );

    exit(EXIT_SUCCESS);
}

/*-----------------------------------------------------------*/
