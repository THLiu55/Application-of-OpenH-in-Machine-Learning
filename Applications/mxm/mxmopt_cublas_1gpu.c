
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
    int deviceId; 
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
   acc_set_device_num(cData.deviceId, acc_device_nvidia);
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
    if (argc != 6)
    {
       fprintf(stderr,
          "Usage: %s <M> <N> <K> <GPU ID> <verbosity>\n",
          argv[0]);
       exit(EXIT_FAILURE);
    }

    int i;
    int M = atoi(argv[1]);
    int N = atoi(argv[2]);
    int K = atoi(argv[3]);
    int gpuid = atoi(argv[4]);
    int verbosity = atoi(argv[5]);

    openh_init();

    int coreid = 2;
    openh_assign_acc_pcpuids(0, &coreid, 1);
    openh_print();

    double *A, *B, *C;
    A = (double*)malloc(sizeof(double)*M*K);
    B = (double*)malloc(sizeof(double)*K*N);
    C = (double*)malloc(sizeof(double)*M*N);

    for (i = 0; i < (M*K); i++)
    {
        A[i] = 0.001;
    }

    for (i = 0; i < (K*N); i++)
    {
        B[i] = 1000.0;
    }

    for (i = 0; i < (M*N); i++)
    {
        C[i] = 0.0;
    }

    struct timeval startt, endt;

    gettimeofday(&startt, NULL);

    ComponentPartition_t* cData = (ComponentPartition_t*)malloc(
                                        sizeof(ComponentPartition_t));
    pthread_t compT;

    int rc;
    cData->cId = 0;
    cData->deviceId = gpuid;
    cData->nT = -1;
    cData->M = M;
    cData->N = N;
    cData->K = K;
    cData->A = A;
    cData->B = B;
    cData->C = C;
    
    printf("Starting gpu component %d pthread.\n", gpuid);

    rc  = pthread_create(&compT, NULL,
                &gpuComponent, (void*)(cData));
    if (rc != 0)
    {
       errno = rc;
       perror("pthread_create");
       exit(EXIT_FAILURE);
    }

    pthread_join(compT, NULL);

    gettimeofday(&endt, NULL);

    double tstart = startt.tv_sec + startt.tv_usec/1000000.;
    double tend = endt.tv_sec + endt.tv_usec/1000000.;

    double s1 = 0.001 * 2.0 * M;
    double s2 = 0.001 * s1 * N;
    double s3 = 0.001 * s2 * K;
    double mflops = s3 / (tend - tstart);

    printf(
        "MXM CUBLAS, M %d, N %d, K %d, Execution time %lf seconds, Speed(GFLOPs) %lf\n",
        M, N, K, (tend - tstart), mflops);

    if (verbosity)
    {
       printf("C diagonal: ");
       for (i = 0; i < M; i++)
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
