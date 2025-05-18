#include "array/matrix2D.h"

/*----------------------------------------------------------------------------
   patolette__Matrix2D

   This file defines functions that work on 2D matrices. The actual
   patolette__Matrix2D definition can be found in "array/matrix2D.h".
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
   Declarations START
-----------------------------------------------------------------------------*/

static patolette__Matrix2D *patolette__Matrix2D_init_base(size_t rows, size_t cols);
static void patolette__Matrix2D_init_data(patolette__Matrix2D *m, const double *data);

/*----------------------------------------------------------------------------
   Declarations END
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
   Internal functions START
-----------------------------------------------------------------------------*/

static patolette__Matrix2D *patolette__Matrix2D_init_base(size_t rows, size_t cols) {
/*----------------------------------------------------------------------------
   Initializes Matrix2D (base).

   @params
   rows - Row count.
   cols - Column count.

   @note
   I can't remember why I split initialization into init_base and
   init_data (maybe reads easier?).
-----------------------------------------------------------------------------*/
    patolette__Matrix2D *m = malloc(sizeof *m);
    m->rows = rows;
    m->cols = cols;
    return m;
}

static void patolette__Matrix2D_init_data(patolette__Matrix2D *m, const double *data) {
/*----------------------------------------------------------------------------
   Initialize Matrix2D (data).

   @params
   m - The Matrix2D.
   data - The matrix's data. If NULL, memory is zero-allocated (all matrix
   cells will evaluate to 0).

   @note
   @param data must be column-major ordered. This decision was made
   to avoid having to transpose when invoking BLAS routines. Did this
   bring any benefit? Probably not; in the end OpenBLAS and Accelerate
   support both orderings with supposedly minimal to no overhead.
   My bad ¯\_(ツ)_/¯

   @note
   I can't remember why I split initialization into init_base and
   init_data (maybe reads easier?).
-----------------------------------------------------------------------------*/
    if (data != NULL) {
        size_t bytes = sizeof(double) * m->rows * m->cols;
        m->data = malloc(bytes);
        memcpy(m->data, data, bytes);
    }
    else {
        m->data = calloc(
            m->rows * m->cols,
            sizeof(double)
        );
    }
}

/*----------------------------------------------------------------------------
   Internal functions END
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
   Exported functions START
-----------------------------------------------------------------------------*/

void patolette__Matrix2D_destroy(patolette__Matrix2D *m) {
/*----------------------------------------------------------------------------
   Destroys a Matrix2D.

   @params
   m - The Matrix2D.
-----------------------------------------------------------------------------*/
    if (m == NULL) {
        return;
    }

    free(m->data);
    free(m);
}

patolette__Matrix2D *patolette__Matrix2D_init(
    size_t rows, 
    size_t cols,
    const double *data
) {
/*----------------------------------------------------------------------------
   Initializes a Matrix2D.

   @params
   rows - Row count.
   cols - Column count.
   data - The matrix's data. If NULL, memory is zero-allocated (all matrix
   cells will evaluate to 0).
-----------------------------------------------------------------------------*/
    patolette__Matrix2D *m = patolette__Matrix2D_init_base(rows, cols);
    patolette__Matrix2D_init_data(m, data);
    return m;
}

patolette__Matrix2D *patolette__Matrix2D_copy(const patolette__Matrix2D *m) {
/*----------------------------------------------------------------------------
   Creates a copy of a Matrix2D.

   @params
   m - The Matrix2D.
-----------------------------------------------------------------------------*/
    return patolette__Matrix2D_init(
        m->rows,
        m->cols,
        m->data
    );
}

patolette__Vector *patolette__Matrix2D_extract_column(
    const patolette__Matrix2D *m,
    size_t column
) {
/*----------------------------------------------------------------------------
   Extracts a column from a Matrix2D.

   @params
   m - The Matrix2D.
   column - The index of the column to extract.

   @example
   If m = | 1 2 3 |
          | 3 0 0 |
          | 2 2 2 | then:
   patolette__Matrix2D_extract_column(m, 1) = | 2, 0, 2 |
-----------------------------------------------------------------------------*/
    size_t rows = m->rows;
    size_t cols = m->cols;
    
    patolette__Vector *col = patolette__Vector_init(cols);
    for (size_t i = 0; i < rows; i++) {
        patolette__Vector_index(col, i) = patolette__Matrix2D_index(m, i, column);
    }

    return col;
}

patolette__Matrix2D *patolette__Matrix2D_extract_rows(
    const patolette__Matrix2D *m,
    const patolette__IndexArray *rows
) {
/*----------------------------------------------------------------------------
   Extracts a set of rows from a Matrix2D.

   @params
   m - The Matrix2D.
   rows - The indexes of the rows to extract.

   @example
   If m = | 1 2 3 |
          | 3 0 0 |
          | 2 2 2 |
      rows = | 0 1 | then:
   patolette__Matrix2D_extract_rows(m, rows) = | 1 2 3 |
                                               | 3 0 0 |
-----------------------------------------------------------------------------*/
    size_t cols = m->cols;
    size_t length = rows->length;

    patolette__Matrix2D *extracted = patolette__Matrix2D_init(length, cols, NULL);
    for (size_t i = 0; i < length; i++) {
        size_t row = patolette__IndexArray_index(rows, i);
        for (size_t j = 0; j < cols; j++) {
            patolette__Matrix2D_index(extracted, i, j) = patolette__Matrix2D_index(
                m,
                row,
                j
            );
        }
    }

    return extracted;
}

patolette__Vector *patolette__Matrix2D_get_vector_mean(const patolette__Matrix2D *m) {
/*----------------------------------------------------------------------------
   Gets the (column) vector mean of a Matrix2D.

   @params
   m - The Matrix2D.

   @example
   If m = | 1 2 3 |
          | 3 0 0 |
          | 2 2 2 | then
   patolette__Matrix2D_get_vector_mean(m) = | 2, 1.333, 1.666 |
-----------------------------------------------------------------------------*/
    size_t rows = m->rows;
    size_t cols = m->cols;

    patolette__Vector *mean = patolette__Vector_init(cols);
    for (size_t j = 0; j < cols; j++) {
        for (size_t i = 0; i < rows; i++) {
            double v = patolette__Matrix2D_index(m, i, j);
            patolette__Vector_index(mean, j) += v;
        }
    }

    double s = 1 / (double)rows;
    patolette__Vector_scale(mean, s);
    return mean;
}

// DEBUG
void patolette__Matrix2D_print(patolette__Matrix2D *m) {
    size_t rows = m->rows;
    size_t cols = m->cols;
    for (size_t i = 0; i < rows; i++) {
        for (size_t j = 0; j < cols; j++) {
            printf("%f", patolette__Matrix2D_index(m, i, j));
            printf("\t");
        }
        printf("\n");
    }
}

/*----------------------------------------------------------------------------
   Exported functions END
-----------------------------------------------------------------------------*/