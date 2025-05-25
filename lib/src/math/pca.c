#include "math/pca.h"

/*----------------------------------------------------------------------------
    This file defines functions to perform PCA (principal component analysis).

    TODO: use as many BLAS routines as possible
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
    Declarations START
-----------------------------------------------------------------------------*/

static patolette__Matrix2D *get_mean_centered_matrix(
    const patolette__Matrix2D *m,
    const patolette__Vector *weights
);

static patolette__Matrix2D *get_vcov(
    const patolette__Matrix2D *m,
    const patolette__Vector *weights
);

/*----------------------------------------------------------------------------
    Declarations END
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
    Internal functions START
-----------------------------------------------------------------------------*/

static patolette__Matrix2D *get_mean_centered_matrix(
    const patolette__Matrix2D *m,
    const patolette__Vector *weights
) {
/*----------------------------------------------------------------------------
    Performs column-wise centering on a Matrix2D.
    The input Matrix2D is not modified.

    @param
    m - The Matrix2D.
    weights - The weight of each row.
-----------------------------------------------------------------------------*/
    size_t rows = m->rows;
    size_t cols = m->cols;

    patolette__Vector *mean = patolette__Matrix2D_get_vector_mean(m, weights);
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

patolette__Matrix2D *get_vcov(
    const patolette__Matrix2D *m,
    const patolette__Vector *weights
) {
/*----------------------------------------------------------------------------
    Gets the weighted variance-covariance matrix of a Matrix2D.
    The input matrix is not modified.

    @param
    m - A Matrix2D treated as a set of samples. Each row a sample,
    each column a feature. In our case, columns represent color channels,
    rows represent colors.
    weights - The weight of each color.
-----------------------------------------------------------------------------*/
    size_t rows = m->rows;
    size_t cols = m->cols;

    patolette__Matrix2D *centered = get_mean_centered_matrix(m, weights);
    patolette__Matrix2D *vcov = patolette__Matrix2D_init(cols, cols, NULL);

    double w_sum = weights == NULL ? (double)rows : patolette__Vector_sum(weights);

    for (size_t j = 0; j < cols; j++) {
        for (size_t k = 0; k < cols; k++) {

            double value = 0;
            for (size_t i = 0; i < rows; i++) {
                double weight = weights == NULL ? 1 : patolette__Vector_index(weights, i);
                double cij = patolette__Matrix2D_index(centered, i, j);
                double cik = patolette__Matrix2D_index(centered, i, k);
                value += weight * cij * cik;
            }

            patolette__Matrix2D_index(vcov, j, k) = value / w_sum;
        }
    }

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

patolette__PCA *patolette__PCA_perform_PCA(
    const patolette__Matrix2D *m,
    const patolette__Vector *weights
) {
/*----------------------------------------------------------------------------
    Performs PCA.

    @params
    m - A Matrix2D treated as a set of samples. Each row a sample,
    each column a feature. In our case, columns represent color channels,
    rows represent colors.
    weights - The weight of each color.
-----------------------------------------------------------------------------*/
    patolette__Matrix2D *vcov = get_vcov(m, weights);
    patolette__PCA *result = patolette__PCA_perform_PCA_vcov(vcov);
    patolette__Matrix2D_destroy(vcov);
    return result;
}

/*----------------------------------------------------------------------------
    Exported functions END
-----------------------------------------------------------------------------*/