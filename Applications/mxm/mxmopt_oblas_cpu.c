
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
   double s3 = s2 * K;

   double mflops = s3 / (tend - tstart);

   printf(
       "CPU ID %d, m %d, n %d, k %d, Execution time %lf seconds, Speed(MFLOPs) %lf\n",
       cpuComponentId, M, N, K, (tend - tstart), mflops);

   pthread_exit(NULL);
}

/*--------------------------------------------------------*/

int main(int argc, char** argv)
{
    if (argc != 3)
    {
       fprintf(stderr,
          "Usage: %s <N> <verbosity>\n"
          "<N> - Matrix dimension\n",
          argv[0]);
       exit(EXIT_FAILURE);
    }

    int i, N = atoi(argv[1]);
    int verbosity = atoi(argv[2]);

    openh_init();

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

    int num_logical_cpuids;
    int *logical_cpuids;
    int rc = openh_get_logical_cpuids(
             &num_logical_cpuids,
             &logical_cpuids);

    if (rc != OPENH_SUCCESS)
    {
       exit(EXIT_FAILURE);
    }

    free(logical_cpuids);

    ComponentPartition_t* cData = (ComponentPartition_t*)malloc(
                                        sizeof(ComponentPartition_t));
    cData[0].cId = 0;
    cData[0].nT = num_logical_cpuids;
    cData[0].M = N;
    cData[0].N = N;
    cData[0].K = N;
    cData[0].A = A;
    cData[0].B = B;
    cData[0].C = C;
    
    printf("Starting cpu component %d pthread, "
           "cpunumrows %d.\n", i, cData[0].M);

    struct timeval startt, endt;

    gettimeofday(&startt, NULL);

    pthread_t compT;
    rc  = pthread_create(&compT, NULL,
                   &cpuComponent, (void*)(cData));
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

    double s1 = 0.001 * 2.0 * N;
    double s2 = 0.001 * s1 * N;
    double s3 = s2 * N;
    double mflops = s3 / (tend - tstart);

    printf(
        "MXM OBLAS, N %d, Execution time %lf seconds, Speed(MFLOPs) %lf\n",
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
