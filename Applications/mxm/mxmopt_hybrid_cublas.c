
/*--------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <openacc.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>
#include <cblas.h>
#include <omp.h>

/*--------------------------------------------------------*/

#include <cuda_runtime.h>
#include <cublas_v2.h>

/*--------------------------------------------------------*/

#include <openh.h>
#include <openherr.h>

/*--------------------------------------------------------*/

typedef struct _ComponentPartition_t { 
    int cId; 
    int nT;
    int M; 
    int N; 
    int K; 
    double* A; 
    double* B;
    double* C;
} ComponentPartition_t;

/*--------------------------------------------------------*/

void *gpuComponent(void *arg) {
   struct timeval start, end;

   gettimeofday(&start, NULL);

   ComponentPartition_t cData = *(ComponentPartition_t*)arg;

   int gpuComponentId = cData.cId;
   int M = cData.M;
   int N = cData.N;
   int K = cData.K;
   
   /* Bind the GPU component */   
   acc_set_device_num(gpuComponentId, acc_device_nvidia);
   openh_bind_acc_self(gpuComponentId);
   
   double* A = cData.A;
   double* B = cData.B; 
   double* C = cData.C;

   printf(
      "GPU Component %d: Performing matrix multiplication of %d x %d and %d x %d matrices.\n",
      gpuComponentId, M, K, K, N);

   cublasHandle_t handle;
   cublasStatus_t status = cublasCreate(&handle);

   if (status != CUBLAS_STATUS_SUCCESS)
   {
      fprintf(stderr, "!!!! CUBLAS initialization error\n");
      pthread_exit(NULL);
   }

#pragma acc data copyin(A[0:M*K], B[0:K*N]) copyout(C[0:M*N])
{
      #pragma acc host_data use_device(A, B, C)
      {
          double alpha = 1.0;
          double beta = 1.0;
          cublasDgemm(handle, CUBLAS_OP_N, CUBLAS_OP_N, M, N, K,
                      &alpha, A, M, B, K, &beta, C, M);
      }
}

   cublasDestroy(handle);

   gettimeofday(&end, NULL);

   double tstart = start.tv_sec + start.tv_usec/1000000.;
   double tend = end.tv_sec + end.tv_usec/1000000.;

   double s1 = 0.001 * 2.0 * M;
   double s2 = 0.001 * s1 * N;
   double s3 = 0.001 * s2 * K;

   double mflops = s3 / (tend - tstart);

   printf(
       "GPU ID %d, m %d, n %d, k %d, Execution time %lf seconds, Speed(GFLOPs) %lf\n",
       gpuComponentId, M, N, K, (tend - tstart), mflops);

   pthread_exit(NULL);
}

/*--------------------------------------------------------*/

int main(int argc, char** argv)
{
    if (argc != 3)
    {
       fprintf(stderr,
          "Usage: %s <N> <verbosity>\n"
          "<N> - Matrix dimension\n"
          "Assume that all GPUs are identical\n",
          argv[0]);
       exit(EXIT_FAILURE);
    }

    int i, N = atoi(argv[1]);
    int verbosity = atoi(argv[2]);

    openh_init();

    int p = openh_get_num_accelerators();

    /*
     * core id 0 is reserved for the master.
     * Try 1, 2...
     */
    for (i = 0; i < p; i++)
    {
        int* closestCpuIds;
        int ncpus;

        openh_get_accelerator_lcpuaffinity(
             i, &closestCpuIds, &ncpus);

        if (ncpus > (i+1))
        {
           openh_assign_acc_pcpuids(i, &closestCpuIds[i+1], 1);
        }
        else
        {
           openh_assign_acc_pcpuids(i, &closestCpuIds[0], 1);
        }

        free(closestCpuIds);
    }

    openh_print();

    double *A, *B, *C;
    int numElements = N*N;

    A = (double*)malloc(sizeof(double)*numElements);
    B = (double*)malloc(sizeof(double)*numElements);
    C = (double*)malloc(sizeof(double)*numElements);

    for (i = 0; i < numElements; i++)
    {
        A[i] = 0.001;
        B[i] = 1000.0;
        C[i] = 0.0;
    }

    struct timeval startt, endt;

    gettimeofday(&startt, NULL);

    ComponentPartition_t* cData = (ComponentPartition_t*)malloc(
                                        sizeof(ComponentPartition_t)*p);
    pthread_t compT[p];

    int rc;
    int start = 0;
    for (i = 0; i < p; i++) {
        cData[i].cId = i;
        cData[i].nT = -1;
        cData[i].M = N/p;
        cData[i].N = N;
        cData[i].K = N;
        cData[i].A = &A[start];
        cData[i].B = B;
        cData[i].C = &C[start];
    
        printf("Starting gpu component %d pthread, "
               "gpunumrows %d, start %d.\n", i, N/p, start);

        rc  = pthread_create(&compT[i], NULL,
                       &gpuComponent, (void*)(&cData[i]));
        if (rc != 0)
        {
           errno = rc;
           perror("pthread_create");
           exit(EXIT_FAILURE);
        }

        start += cData[i].M * N;
    }
    
    for (i = 0; i < p; i++) {
        pthread_join(compT[i], NULL);
    }

    gettimeofday(&endt, NULL);

    double tstart = startt.tv_sec + startt.tv_usec/1000000.;
    double tend = endt.tv_sec + endt.tv_usec/1000000.;

    double s1 = 0.001 * 2.0 * N;
    double s2 = 0.001 * s1 * N;
    double s3 = 0.001 * s2 * N;
    double mflops = s3 / (tend - tstart);

    printf(
        "MXM, N %d, Execution time %lf seconds, Speed(GFLOPs) %lf\n",
        N, (tend - tstart), mflops);

    if (verbosity)
    {
       printf("C diagonal: ");
       for (i = 0; i < N; i++)
       {
           printf("%lf ", C[i*N + i]);
       }
       printf("\n");
    }

    free(cData);

    openh_finalize();

    free(A);
    free(B);
    free(C);

    exit(EXIT_SUCCESS);
}

/*--------------------------------------------------------*/
