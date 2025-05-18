#include "math/pca.h"

/*----------------------------------------------------------------------------
    This file defines functions to perform PCA (principal component analysis).

    Only the variance-covariance matrix multiplication is implemented via a
    BLAS routine, but the rest of the functions could also be adapted if needed
    (for better performance).
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
    Declarations START
-----------------------------------------------------------------------------*/

static patolette__Matrix2D *get_mean_centered_matrix(const patolette__Matrix2D *m);
static patolette__Matrix2D *get_vcov(const patolette__Matrix2D *m);

/*----------------------------------------------------------------------------
    Declarations END
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
    Internal functions START
-----------------------------------------------------------------------------*/

static patolette__Matrix2D *get_mean_centered_matrix(const patolette__Matrix2D *m) {
/*----------------------------------------------------------------------------
    Performs column-wise centering on a Matrix2D.
    The input Matrix2D is not modified.

    @param
    m - The Matrix2D.
-----------------------------------------------------------------------------*/
    size_t rows = m->rows;
    size_t cols = m->cols;

    patolette__Vector *mean = patolette__Matrix2D_get_vector_mean(m);
    patolette__Matrix2D *centered = patolette__Matrix2D_copy(m);

    for (size_t j = 0; j < cols; j++) {
        for (size_t i = 0; i < rows; i++) {
            double cm = patolette__Vector_index(mean, j);
            patolette__Matrix2D_index(centered, i, j) -= cm;
        }
    }

    patolette__Vector_destroy(mean);
    return centered;
}

static patolette__Matrix2D *get_vcov(const patolette__Matrix2D *m) {
/*----------------------------------------------------------------------------
    Gets the variance-covariance matrix of a Matrix2D.
    The input matrix is not modified. The output matrix's upper triangle
    is not populated.

    @param
    m - The Matrix2D.
-----------------------------------------------------------------------------*/
    size_t rows = m->rows;
    size_t cols = m->cols;

    patolette__Matrix2D *centered = get_mean_centered_matrix(m);
    patolette__Matrix2D *vcov = patolette__Matrix2D_init(cols, cols, NULL);

    blasint n = (blasint)cols;
    blasint k = (blasint)rows;
    double alpha = 1 / (double)(rows - 1);
    double *a = centered->data;
    blasint lda = (blasint)rows;
    blasint ldc = (blasint)cols;
    double *c = vcov->data;

    cblas_dsyrk(
        CblasColMajor,
        CblasLower,
        CblasTrans,
        n,
        k,
        alpha,
        a,
        lda,
        0,
        c,
        ldc
    );

    patolette__Matrix2D_destroy(centered);
    return vcov;
}

/*----------------------------------------------------------------------------
    Internal functions END
-----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
    Exported functions START
-----------------------------------------------------------------------------*/

void patolette__PCA_destroy(patolette__PCA *pca) {
/*----------------------------------------------------------------------------
    Destroys a PCA object.

    @params
    pca - The PCA object.
-----------------------------------------------------------------------------*/
    patolette__Vector_destroy(pca->axis);
    free(pca);
}

patolette__PCA *patolette__PCA_perform_PCA_vcov(patolette__Matrix2D *vcov) {
/*----------------------------------------------------------------------------
    Performs PCA directly on a variance-covariance matrix.
    The input matrix is modified. Check patolette__EIGEN_solve

    @params
    vcov - The variance-covariance matrix
-----------------------------------------------------------------------------*/
    patolette__Vector *evals = patolette__EIGEN_solve(vcov);
    if (evals == NULL) {
        return NULL;
    }

    patolette__PCA *result = malloc(sizeof *result);
    size_t eval_idx = vcov->cols - 1;

    result->axis = patolette__Matrix2D_extract_column(vcov, eval_idx);

    result->explained_variance = 0;
    double sum = patolette__Vector_sum(evals);
    if (sum > patolette__DELTA) {
        double eval = patolette__Vector_index(evals, eval_idx);
        result->explained_variance = eval / sum;
    }

    patolette__Vector_destroy(evals);
    return result;
}

patolette__PCA *patolette__PCA_perform_PCA(const patolette__Matrix2D *m) {
/*----------------------------------------------------------------------------
    Performs PCA.

    @params
    m - A Matrix2D treated as a set of samples. Each row a sample,
    each column a feature. In our case, columns represent color channels,
    rows represent colors.
-----------------------------------------------------------------------------*/
    patolette__Matrix2D *vcov = get_vcov(m);
    patolette__PCA *result = patolette__PCA_perform_PCA_vcov(vcov);
    patolette__Matrix2D_destroy(vcov);
    return result;
}

/*----------------------------------------------------------------------------
    Exported functions END
-----------------------------------------------------------------------------*/