
/*--------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <openacc.h>
#include <pthread.h>
#include <errno.h>

/*--------------------------------------------------------*/

#include <openh.h>

/*--------------------------------------------------------*/

typedef struct _ComponentPartition_t { 
    int cId; 
    int N; 
    double* A; 
    double* B;
    double* C;
} ComponentPartition_t;

/*--------------------------------------------------------*/

void *gpuComponent(void *arg) {
   ComponentPartition_t cData = *(ComponentPartition_t*)arg;

   int gpuComponentId = cData.cId;
   int N = cData.N;
   
   /* Bind the GPU component */   
   acc_set_device_num(gpuComponentId, acc_device_nvidia);
   openh_bind_acc_self(gpuComponentId);
   
   int i, j, k;

   double* A = cData.A;
   double* B = cData.B; 
   double* C = cData.C;

   printf(
      "GPU Component: Performing matrix multiplication of %d x %d matrices.\n",
      N, N);

#pragma acc data copyin(A[0:N*N], B[0:N*N]) copyout(C[0:N*N])
{
    #pragma acc parallel loop gang
    for (j = 0; j < N; j++)
    {
        #pragma acc loop vector
        for (i = 0; i < N; i++)
        {
            double sum = 0.0;
            for (k = 0; k < N; k++)
            {
               sum += A[i*N + k] * B[k*N + j];
            }
            C[i*N + j] = sum;
        }
    }
}

   pthread_exit(NULL);
}

/*--------------------------------------------------------*/

int main(int argc, char** argv)
{
    if (argc != 3)
    {
       fprintf(stderr,
          "Usage: %s <N> <gpu core>\n"
          "<N> - Matrix dimension\n",
          argv[0]);
       exit(EXIT_FAILURE);
    }

    int i, N = atoi(argv[1]);
    int cpuid = atoi(argv[2]);

    openh_init();

    openh_assign_acc_pcpuids(0, &cpuid, 1);

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

    ComponentPartition_t* cData = (ComponentPartition_t*)malloc(
                                        sizeof(ComponentPartition_t));

    cData->cId = 0;
    cData->N = N;
    cData->A = A;
    cData->B = B;
    cData->C = C;
    
    printf("Starting gpu component pthread.\n");

    pthread_t gpuT;
    int rc  = pthread_create(&gpuT, NULL,
                   &gpuComponent, (void*)cData);
    if (rc != 0)
    {
       errno = rc;
       perror("pthread_create");
       exit(EXIT_FAILURE);
    }
    
    pthread_join(gpuT, NULL);

    free(cData);

    printf("C diagonal: ");
    for (i = 0; i < N; i++)
    {
        printf("%lf ", C[i*N + i]);
    }
    printf("\n");

    printf("Finalizing openh runtime.\n");

    openh_finalize();

    free(A);
    free(B);
    free(C);

    exit(EXIT_SUCCESS);
}

/*--------------------------------------------------------*/
