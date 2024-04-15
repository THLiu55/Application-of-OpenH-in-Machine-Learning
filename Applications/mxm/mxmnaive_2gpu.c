
/*--------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <openacc.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>

/*--------------------------------------------------------*/

#include <openh.h>

/*--------------------------------------------------------*/

typedef struct _ComponentPartition_t { 
    int cId; 
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
          "Usage: %s <N> <gpu core 0> <gpu core 1> <verbosity>\n"
          "<N> - Matrix dimension\n",
          argv[0]);
       exit(EXIT_FAILURE);
    }

    int i, N = atoi(argv[1]);
    int core0 = atoi(argv[2]);
    int core1 = atoi(argv[3]);
    int verbosity = atoi(argv[4]);

    openh_init();

    openh_assign_acc_pcpuids(0, &core0, 1);
    openh_assign_acc_pcpuids(1, &core1, 1);

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
                                        sizeof(ComponentPartition_t)*2);

    cData[0].cId = 0;
    cData[0].M = N/2;
    cData[0].N = N;
    cData[0].K = N;
    cData[0].A = A;
    cData[0].B = B;
    cData[0].C = C;
    
    printf("Starting gpu component 1 pthread.\n");

    pthread_t gpuT[2];
    int rc  = pthread_create(&gpuT[0], NULL,
                   &gpuComponent, (void*)(&cData[0]));
    if (rc != 0)
    {
       errno = rc;
       perror("pthread_create");
       exit(EXIT_FAILURE);
    }
    
    printf("Starting gpu component 2 pthread.\n");

    cData[1].cId = 1;
    cData[1].M = N/2;
    cData[1].N = N;
    cData[1].K = N;
    cData[1].A = &A[N * (N/2)];
    cData[1].B = B;
    cData[1].C = &C[N * (N/2)];
    
    rc  = pthread_create(&gpuT[1], NULL,
                   &gpuComponent, (void*)(&cData[1]));
    if (rc != 0)
    {
       errno = rc;
       perror("pthread_create");
       exit(EXIT_FAILURE);
    }
    
    pthread_join(gpuT[0], NULL);
    pthread_join(gpuT[1], NULL);

    gettimeofday(&endt, NULL);

    double tstart = startt.tv_sec + startt.tv_usec/1000000.;
    double tend = endt.tv_sec + endt.tv_usec/1000000.;

    double s1 = 0.001 * 2.0 * N;
    double s2 = 0.001 * s1 * N;
    double s3 = s2 * N;
    double mflops = s3 / (tend - tstart);

    printf(
        "MXM, N %d, Execution time %lf seconds, Speed(MFLOPs) %lf\n",
        N, (tend - tstart), mflops);

    free(cData);

    if (verbosity)
    {
       printf("C diagonal: ");
       for (i = 0; i < N; i++)
       {
           printf("%lf ", C[i*N + i]);
       }
       printf("\n");
    }

    printf("Finalizing openh runtime.\n");

    openh_finalize();

    free(A);
    free(B);
    free(C);

    exit(EXIT_SUCCESS);
}

/*--------------------------------------------------------*/
