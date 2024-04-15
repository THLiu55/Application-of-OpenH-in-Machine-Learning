
/*--------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>

#include <openh.h>

/*--------------------------------------------------------*/

typedef struct _ComponentPartition_t { 
    int cId; 
    int nT;
    int N; 
    double* A; 
    double* B;
    double* C;
} ComponentPartition_t;

/*--------------------------------------------------------*/

void *cpuComponent(void *arg) {
   ComponentPartition_t cData = *(ComponentPartition_t*)arg;

   int cpuComponentId = cData.cId;
   int nT = cData.nT;
   int N = cData.N;
   
   /* Bind the CPU component */   
   openh_bind_cpu_self(cpuComponentId);
   
   int i, j, k;

   double* A = cData.A;
   double* B = cData.B; 
   double* C = cData.C;

   printf(
      "CPU Component: Performing matrix multiplication of %d x %d matrices.\n",
      N, N);

#pragma omp parallel num_threads(nT)
{
    #pragma omp for collapse(2) private(j,k) 
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
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
    if (argc != 4)
    {
       fprintf(stderr,
          "Usage: %s <N> <cstart> <cend>\n"
          "<N> - Matrix dimension\n"
          "<cstart> - starting core to bind\n"
          "<cend> - end core to bind.\n",
          argv[0]);
       exit(EXIT_FAILURE);
    }

    int i, N = atoi(argv[1]);
    int cStart = atoi(argv[2]);
    int cEnd = atoi(argv[3]);

    if (cEnd < cStart)
    {
       fprintf(
          stderr, "Binding core range (%d,%d) is invalid.\n",
          cStart, cEnd);
       exit(EXIT_FAILURE);
    }

    openh_init();

    int ncpus = cEnd - cStart + 1;
    int* cpuIds = (int*)malloc(sizeof(int)*ncpus);

    for (i = 0; i < ncpus; i++)
    {
        cpuIds[i] = cStart + i;
    }

    openh_assign_cpu_pcpuids(0, cpuIds, ncpus);

    free(cpuIds);

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
    cData->nT = ncpus;
    cData->N = N;
    cData->A = A;
    cData->B = B;
    cData->C = C;
    
    printf("Starting cpu component pthread.\n");

    pthread_t cpuT;
    int rc  = pthread_create(&cpuT, NULL,
                   &cpuComponent, (void*)cData);
    if (rc != 0)
    {
       errno = rc;
       perror("pthread_create");
       exit(EXIT_FAILURE);
    }
    
    pthread_join(cpuT, NULL);

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
