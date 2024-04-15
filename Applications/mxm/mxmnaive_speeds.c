
/*--------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <openacc.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>

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
   
   int i, j, k;

   double* A = cData.A;
   double* B = cData.B; 
   double* C = cData.C;

   printf(
      "CPU Component %d: Performing matrix multiplication of %d x %d and %d x %d matrices.\n",
      cpuComponentId, M, K, K, N);

#pragma omp parallel num_threads(nT)
{
    #pragma omp for collapse(2) private(j,k) 
    for (i = 0; i < M; i++)
    {
        for (j = 0; j < N; j++)
	    {
           double sum = 0.0;
           for (k = 0; k < K; k++)
           {
              sum += A[i*K + k] * B[k*N + j];
           }
           C[i*N + j] = sum;
        }           
    }
}

   gettimeofday(&end, NULL);

   double tstart = start.tv_sec + start.tv_usec/1000000.;
   double tend = end.tv_sec + end.tv_usec/1000000.;

   double s1 = 0.001 * 2.0 * M;
   double s2 = 0.001 * s1 * N;
   double s3 = s2 * K;

   double mflops = s3 / (tend - tstart);

   printf(
       "CPU ID %d, m %d, n %d, k %d, Execution time %lf seconds, Speed(MFLOPs) %lf\n",
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
   
   int i, j, k;

   double* A = cData.A;
   double* B = cData.B; 
   double* C = cData.C;

   printf(
      "GPU Component %d: Performing matrix multiplication of %d x %d and %d x %d matrices.\n",
      gpuComponentId, M, K, K, N);

#pragma acc data copyin(A[0:M*K], B[0:K*N]) copyout(C[0:M*N])
{
    #pragma acc parallel loop gang
    for (j = 0; j < N; j++)
    {
        #pragma acc loop vector
        for (i = 0; i < M; i++)
        {
            double sum = 0.0;
            for (k = 0; k < K; k++)
            {
               sum += A[i*K + k] * B[k*N + j];
            }
            C[i*N + j] = sum;
        }
    }
}

   gettimeofday(&end, NULL);

   double tstart = start.tv_sec + start.tv_usec/1000000.;
   double tend = end.tv_sec + end.tv_usec/1000000.;

   double s1 = 0.001 * 2.0 * M;
   double s2 = 0.001 * s1 * N;
   double s3 = s2 * K;

   double mflops = s3 / (tend - tstart);

   printf(
       "GPU ID %d, m %d, n %d, k %d, Execution time %lf seconds, Speed(MFLOPs) %lf\n",
       gpuComponentId, M, N, K, (tend - tstart), mflops);

   pthread_exit(NULL);
}

/*--------------------------------------------------------*/

int main(int argc, char** argv)
{
    if (argc != 5)
    {
       fprintf(stderr,
          "Usage: %s <M> <N> <K> <verbosity>\n"
          "<M> - Matrix dimension\n"
          "<N> - Matrix dimension\n"
          "<K> - Matrix dimension\n"
          "<verbosity> - Verbosity\n",
          argv[0]);
       exit(EXIT_FAILURE);
    }

    int i;
    int M = atoi(argv[1]);
    int N = atoi(argv[2]);
    int K = atoi(argv[3]);
    int verbosity = atoi(argv[4]);

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

    ComponentPartition_t* cData = (ComponentPartition_t*)malloc(
                                        sizeof(ComponentPartition_t)*p);
    pthread_t compT[p];

    int rc;
    for (i = 0; i < p-1; i++) {
        cData[i].cId = i;
        cData[i].nT = -1;
        cData[i].M = M;
        cData[i].N = N;
        cData[i].K = K;
        cData[i].A = A;
        cData[i].B = B;
        cData[i].C = C;
    
        printf("Starting gpu component %d pthread.\n", i);

        rc  = pthread_create(&compT[i], NULL,
                       &gpuComponent, (void*)(&cData[i]));
        if (rc != 0)
        {
           errno = rc;
           perror("pthread_create");
           exit(EXIT_FAILURE);
        }
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
    cData[p-1].M = M;
    cData[p-1].N = N;
    cData[p-1].K = K;
    cData[p-1].A = A;
    cData[p-1].B = B;
    cData[p-1].C = C;
    
    printf("Starting cpu component %d pthread.\n", 0);

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

    double s1 = 0.001 * 2.0 * M;
    double s2 = 0.001 * s1 * N;
    double s3 = s2 * K;
    double mflops = s3 / (tend - tstart);

    printf(
        "MXM, N %d, Execution time %lf seconds, Speed(MFLOPs) %lf\n",
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
