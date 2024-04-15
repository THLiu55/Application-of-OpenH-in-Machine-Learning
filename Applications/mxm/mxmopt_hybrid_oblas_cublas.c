
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

void *cpuComponent(void *arg) {
   struct timeval start, end;

   gettimeofday(&start, NULL);

   ComponentPartition_t cData = *(ComponentPartition_t*)arg;

   int cpuComponentId = cData.cId;
   int nT = cData.nT;
   int M = cData.M;
   int N = cData.N;
   int K = cData.K;
   
   /* Bind the CPU component */   
   openh_bind_cpu_self(cpuComponentId);
   
   double* A = cData.A;
   double* B = cData.B; 
   double* C = cData.C;

   printf(
      "CPU Component %d: Performing matrix multiplication of %d x %d and %d x %d matrices.\n",
      cpuComponentId, M, K, K, N);

   openblas_set_num_threads(nT);
   goto_set_num_threads(nT);
   omp_set_num_threads(nT);

   cblas_dgemm(CblasRowMajor, CblasNoTrans,
               CblasNoTrans, M, N, K, 1.0,
               A, K, B, N, 1.0, C, N);

   gettimeofday(&end, NULL);

   double tstart = start.tv_sec + start.tv_usec/1000000.;
   double tend = end.tv_sec + end.tv_usec/1000000.;

   double s1 = 0.001 * 2.0 * M;
   double s2 = 0.001 * s1 * N;
   double s3 = 0.001 * s2 * K;

   double mflops = s3 / (tend - tstart);

   printf(
       "CPU ID %d, m %d, n %d, k %d, Execution time %lf seconds, Speed(GFLOPs) %lf\n",
       cpuComponentId, M, N, K, (tend - tstart), mflops);

   pthread_exit(NULL);
}

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
    if (argc != 4)
    {
       fprintf(stderr,
          "Usage: %s <N> <sratio> <verbosity>\n"
          "<N> - Matrix dimension\n"
          "<sratio> - Workload distribution between one CPU and one GPU\n"
          "Assume that all GPUs are identical\n",
          argv[0]);
       exit(EXIT_FAILURE);
    }

    int i, N = atoi(argv[1]);
    double sratio = atof(argv[2]);
    int verbosity = atoi(argv[3]);

    openh_init();

    int nacc = openh_get_num_accelerators();
    int p = nacc + 1;

    /*
     * core id 0 is reserved for the master.
     * Try 1, 2...
     */
    for (i = 0; i < nacc; i++)
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

    openh_assign_cpu_free_pcpuids(0);

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

    int gpunumrows;
    double cpupart = 1.0;
    double gpupart = 1.0 / sratio;
    double sumparts = cpupart;
    for (i = 0; i < p-1; i++)
    {
        sumparts += gpupart;
    }

    cpupart = (cpupart / sumparts);
    gpupart = (gpupart / sumparts);
    gpunumrows = gpupart * N;

    ComponentPartition_t* cData = (ComponentPartition_t*)malloc(
                                        sizeof(ComponentPartition_t)*p);
    pthread_t compT[p];

    int rc;
    int start = 0;
    for (i = 0; i < p-1; i++) {
        cData[i].cId = i;
        cData[i].nT = -1;
        cData[i].M = gpunumrows;
        cData[i].N = N;
        cData[i].K = N;
        cData[i].A = &A[start];
        cData[i].B = B;
        cData[i].C = &C[start];
    
        printf("Starting gpu component %d pthread, "
               "gpunumrows %d, start %d.\n", i, gpunumrows, start);

        rc  = pthread_create(&compT[i], NULL,
                       &gpuComponent, (void*)(&cData[i]));
        if (rc != 0)
        {
           errno = rc;
           perror("pthread_create");
           exit(EXIT_FAILURE);
        }

        start += gpunumrows * N;
    }
    
    int num_logical_cpuids;
    int *logical_cpuids;
    rc = openh_get_logical_cpuids(
             &num_logical_cpuids,
             &logical_cpuids);

    if (rc != OPENH_SUCCESS)
    {
       exit(EXIT_FAILURE);
    }

    free(logical_cpuids);

    cData[p-1].cId = 0;
    cData[p-1].nT = num_logical_cpuids - nacc;
    cData[p-1].M = N - gpunumrows * (p - 1);
    cData[p-1].N = N;
    cData[p-1].K = N;
    cData[p-1].A = &A[start];
    cData[p-1].B = B;
    cData[p-1].C = &C[start];
    
    printf("Starting cpu component %d pthread, "
           "cpunumrows %d, start %d.\n", i, cData[p-1].M, start);

    struct timeval startt, endt;

    gettimeofday(&startt, NULL);

    rc  = pthread_create(&compT[p-1], NULL,
                   &cpuComponent, (void*)(&cData[p-1]));
    if (rc != 0)
    {
       errno = rc;
       perror("pthread_create");
       exit(EXIT_FAILURE);
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
