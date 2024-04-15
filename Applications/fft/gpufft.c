
/*----------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include <string.h>
#include <malloc.h>

/*----------------------------------------------------------------*/

#include "gpufft.h"

/*----------------------------------------------------------------*/

#define CUDA_ERROR 1
#define CUFFT_ERROR 2

/*----------------------------------------------------------------*/

#define NUM_STREAMS 3

/*----------------------------------------------------------------*/

#define CHECK_ERROR(ret){ \
    int retVal = ret; \
    if(ret != 0){ \
        return retVal; \
    } \
}

#define CUDA_ERROR_CHECK(res) { \
    cudaError_t result = res; \
    if(result != cudaSuccess){ \
        printf("CUDA error %s on line %d in file %s\n", cudaGetErrorName(result), __LINE__, __FILE__); \
        cudaDeviceReset(); \
        return CUDA_ERROR; \
    } \
}

#define CUFFT_ERROR_CHECK(res){ \
    cufftResult result = res; \
    if(result != CUFFT_SUCCESS){ \
        printf("CUFFT error %d on line %d in file %s\n", result, __LINE__, __FILE__); \
        cudaDeviceReset(); \
        return CUFFT_ERROR; \
    } \
}

/*----------------------------------------------------------------*/

double min(double x, double y)
{
    return (x < y) ? x : y;
}

/*----------------------------------------------------------------*/

int cufft2dMany(
    const int direction, cufftDoubleComplex *X,
    int M, int N)
{
    cufftDoubleComplex *gpuSignal;
    size_t XSsize = M * N * sizeof(cufftDoubleComplex);

    CUDA_ERROR_CHECK(cudaMalloc((void **)&gpuSignal, XSsize));
    CUDA_ERROR_CHECK(cudaMemcpy(gpuSignal, X, XSsize, cudaMemcpyHostToDevice));

    cufftHandle plan;
    int rank = 1;
    int batch = M;
    int s[] = {N};
    int idist = N;
    int odist = N;
    int istride = 1;
    int ostride = 1;
    int *inembed = s;
    int *onembed = s;

    CUFFT_ERROR_CHECK(cufftPlanMany(&plan, rank, s, 
                inembed, istride, idist, onembed, ostride, odist, CUFFT_Z2Z, batch));

    CUFFT_ERROR_CHECK(cufftExecZ2Z(plan, gpuSignal, gpuSignal, direction));

    CUDA_ERROR_CHECK(cudaMemcpy(X, gpuSignal, XSsize, cudaMemcpyDeviceToHost));

    CUFFT_ERROR_CHECK(cufftDestroy(plan));

    CUDA_ERROR_CHECK(cudaFree(gpuSignal));

    return 0;
}

/*----------------------------------------------------------------*/

int gpufftw1dns(
    const int direction, const int m, const int n,
    cufftDoubleComplex* X
)
{
    CHECK_ERROR(cufft2dMany(direction, X, m, n));
    return 0;
}

/*----------------------------------------------------------------*/

int gpufftw1ds(
    const int direction, const int M, const int N,
    cufftDoubleComplex* X,
    int* registerMemory
)
{
    int batchSize = 1;
    int numTransforms = M;
    size_t workAreaSize = batchSize * N * sizeof(cufftDoubleComplex);

    // Register signal as pinned memory to allow async memcopies to and from host
    if (*registerMemory == 0)
    {
       CUDA_ERROR_CHECK(cudaHostRegister(X, M * N * sizeof(cufftDoubleComplex), cudaHostRegisterPortable));
       *registerMemory = 1;
    }

    cufftDoubleComplex* d_workAreas[NUM_STREAMS];
    for(int i = 0; i < NUM_STREAMS; i++) {
        CUDA_ERROR_CHECK(cudaMalloc((void**)&d_workAreas[i], workAreaSize));
    }

    // Allocate work areas, create streams and plans
    int numPlans = NUM_STREAMS;
    cudaStream_t streams[NUM_STREAMS];
    cufftHandle* plans = (cufftHandle*) malloc(sizeof(cufftHandle) * numPlans);
    int s[] = {N};
    int *inembed = s;
    int *onembed = s;
    for(int i = 0; i < NUM_STREAMS; i++){
        CUDA_ERROR_CHECK(cudaStreamCreate(&streams[i]));
        CUFFT_ERROR_CHECK(cufftPlanMany(&plans[i], 1, s, inembed, 1, N, onembed, 1, N, CUFFT_Z2Z, batchSize));
        CUFFT_ERROR_CHECK(cufftSetStream(plans[i], streams[i]));
    }

    for(int i = 0; i < numTransforms; i++){
        int currentBatchSize = min(M - batchSize * i, batchSize);

        // Divide each batch evenly between each stream
        int streamUsed = (currentBatchSize == batchSize) ? (i % NUM_STREAMS) : 0;

        // if the number of 1D-ffts in the transpose is not == batchSize,
        // we allocate it to the specially created plan for the remaining 1d-ffts
        int planIndex =  (currentBatchSize == batchSize) ? (i % NUM_STREAMS) : NUM_STREAMS;

        cufftDoubleComplex* batchStartLocation = X + (i * batchSize * N);

        CUDA_ERROR_CHECK(cudaMemcpyAsync(d_workAreas[streamUsed], batchStartLocation, currentBatchSize * N * sizeof(cufftDoubleComplex), cudaMemcpyHostToDevice, streams[streamUsed]));

        CUFFT_ERROR_CHECK(cufftExecZ2Z(plans[planIndex], (cufftDoubleComplex*)d_workAreas[streamUsed], (cufftDoubleComplex*)d_workAreas[streamUsed], direction));

        CUDA_ERROR_CHECK(cudaMemcpyAsync(batchStartLocation, d_workAreas[streamUsed], currentBatchSize * N * sizeof(cufftDoubleComplex), cudaMemcpyDeviceToHost, streams[streamUsed]));
    }

    cudaDeviceSynchronize();

    for(int i = 0; i < NUM_STREAMS; i++){
        CUDA_ERROR_CHECK(cudaFree(d_workAreas[i]));
    }

    for(int i = 0; i < NUM_STREAMS; i++){
        cudaStreamDestroy(streams[i]);
    }

    for(int i = 0; i < numPlans; i++){
        CUFFT_ERROR_CHECK(cufftDestroy(plans[i]));
    }

    free(plans);
    
    return 0;
}

/*----------------------------------------------------------------*/
