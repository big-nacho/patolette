#include "math/eigen.h"

/*----------------------------------------------------------------------------
    Eigen solver based on LAPACK's dsyev.
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
    Declarations START
-----------------------------------------------------------------------------*/

static void dsyev(
    const char *jobz,
    const char *uplo,
    const int *n,
    double *a,
    const int *lda,
    double *w,
    double *work,
    const int *lwork,
    int *info
);

/*----------------------------------------------------------------------------
    Declarations END
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
    Internal functions START
-----------------------------------------------------------------------------*/

static void dsyev(
    const char *jobz,
    const char *uplo,
    const int *n,
    double *a,
    const int *lda,
    double *w,
    double *work,
    const int *lwork,
    int *info
) {
/*----------------------------------------------------------------------------
    Computes eigenvalues and eigenvectors of a symmetric matrix.
    This is just a wrapper to account for an interface difference in dsyev
    between OpenBLAS and Accelerate.
    dsyev: https://www.netlib.org/lapack/explore-html-3.6.1/d2/d8a/group__double_s_yeigen_ga442c43fca5493590f8f26cf42fed4044.html
-----------------------------------------------------------------------------*/
    dsyev_(
        jobz,
        uplo,
        n,
        a,
        lda,
        w,
        work,
        lwork,
        info
#if !defined(USE_ACCELERATE) && defined(LAPACK_FORTRAN_STRLEN_END)
        /*----------------------------------------------------------------------------
             See issues:
             https://github.com/OpenMathLib/OpenBLAS/issues/3877
             https://github.com/OpenMathLib/OpenBLAS/issues/2154
             https://github.com/Reference-LAPACK/lapack/issues/339
        -----------------------------------------------------------------------------*/
        ,
        1,
        1
#endif
    );
}

/*----------------------------------------------------------------------------
    Internal functions END
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
    Exported functions START
-----------------------------------------------------------------------------*/

patolette__Vector *patolette__EIGEN_solve(patolette__Matrix2D *m) {
/*----------------------------------------------------------------------------
    Computes eigenvalues and eigenvectors of a symmetric matrix.

    @params
    m - A symmetric 2D matrix. Only the lower triangle needs to be
    populated. The contents of the matrix are destroyed, and it is
    populated with the computed eigenvectors (as columns) in ascending
    order (left to right) based on their corresponding eigenvalues.
-----------------------------------------------------------------------------*/
    size_t cols = m->cols;

    char jobz = 'V';
    char uplo = 'L';
    int n = (int)cols;
    double *a = m->data;
    int lda = n;
    double *w = NULL;
    double qwork[1];
    int lwork = -1;
    int info = 0;

    dsyev(
        &jobz, 
        &uplo, 
        &n, 
        a, 
        &lda, 
        w, 
        qwork, 
        &lwork, 
        &info
    );

    if (info != 0) {
        return NULL;
    }

    patolette__Vector *evals = patolette__Vector_init(cols);
    w = evals->data;

    lwork = (int)qwork[0];
    double *work = malloc(lwork);

    dsyev(
        &jobz, 
        &uplo, 
        &n, a, 
        &lda, 
        w, 
        work, 
        &lwork, 
        &info
    );

    free(work);
    return evals;
}

/*----------------------------------------------------------------------------
    Exported functions END
-----------------------------------------------------------------------------*/