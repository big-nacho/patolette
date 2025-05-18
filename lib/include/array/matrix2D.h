#pragma once

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "array/array.h"

#include "vector.h"

/*----------------------------------------------------------------------------
   patolette__Matrix2D

   A container to perform 2D indexing of doubles.
-----------------------------------------------------------------------------*/

typedef struct patolette__Matrix2D {
    // Pointer to the underlying data (column major order!)
    double *data;

    // Row count
    size_t rows;

    // Column count
    size_t cols;
} patolette__Matrix2D;

#define patolette__Matrix2D_index(m, row, col) ((m->data) [((col) * (m->rows)) + (row)])

void patolette__Matrix2D_destroy(patolette__Matrix2D *m);
patolette__Matrix2D *patolette__Matrix2D_init(size_t rows, size_t cols, const double *data);
patolette__Matrix2D *patolette__Matrix2D_copy(const patolette__Matrix2D *m);
patolette__Vector *patolette__Matrix2D_extract_column(const patolette__Matrix2D *m, size_t c);
patolette__Matrix2D *patolette__Matrix2D_extract_rows(
    const patolette__Matrix2D *m,
    const patolette__IndexArray *rows
);
patolette__Vector *patolette__Matrix2D_get_vector_mean(const patolette__Matrix2D *m);

// DEBUG
void patolette__Matrix2D_print(patolette__Matrix2D *m);