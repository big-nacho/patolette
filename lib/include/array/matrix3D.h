#pragma once

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/*----------------------------------------------------------------------------
   patolette__Matrix3D

   A container to perform 3D indexing of doubles.
-----------------------------------------------------------------------------*/

typedef struct patolette__Matrix3D {
    // Pointer to the underlying data
    double *data;

    // x dimension size
    size_t xDim;

    // y dimension size
    size_t yDim;

    // z dimension size
    size_t zDim;
} patolette__Matrix3D;

#define patolette__Matrix3D_resolve_index(m, x, y, z) (\
    ((z) * m->xDim * m->yDim) +\
    ((y) * m->xDim) +\
    ((x))\
)

#define patolette__Matrix3D_index(m, x, y, z) (m->data[patolette__Matrix3D_resolve_index(m, x, y, z)])

void patolette__Matrix3D_destroy(patolette__Matrix3D *m);
patolette__Matrix3D *patolette__Matrix3D_init(size_t xDim, size_t yDim, size_t zDim);