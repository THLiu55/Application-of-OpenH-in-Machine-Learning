
/*----------------------------------------------------------------*/

#include "cpufft.h"

/*----------------------------------------------------------------*/

int cpufftw1d(
    const int sign, const int m, const int n,
    fftw_complex* X, fftw_complex* Y
)
{
    /*
     * 1D row FFTs
     */
    int rank = 1;
    int howmany = m;
    int s[] = {n};
    int idist = n;
    int odist = n;
    int istride = 1;
    int ostride = 1;
    int *inembed = s;
    int *onembed = s;

    fftw_plan my_plan = fftw_plan_many_dft(
                            rank, s, howmany,
                            X, inembed, istride, idist,
                            Y, onembed, ostride, odist,
                            sign, FFTW_ESTIMATE);

    fftw_execute(my_plan);

    fftw_destroy_plan(my_plan);

    return 0;
}

/*----------------------------------------------------------------*/
