#include "array/matrix3D.h"

/*----------------------------------------------------------------------------
   patolette__Matrix3D

   This file defines functions that work on 3D matrices. The actual
   patolette__Matrix3D definition can be found in "array/matrix3D.h".
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
   Exported functions START
-----------------------------------------------------------------------------*/

void patolette__Matrix3D_destroy(patolette__Matrix3D *m) {
/*----------------------------------------------------------------------------
   Destroys a Matrix3D.

   @params
   m - The Matrix3D.
-----------------------------------------------------------------------------*/
    if (m == NULL) {
        return;
    }

    free(m->data);
    free(m);
}

patolette__Matrix3D *patolette__Matrix3D_init(size_t xDim, size_t yDim, size_t zDim) {
/*----------------------------------------------------------------------------
   Initializes a Matrix3D.
   Memory is zero-allocated (all matrix cells evaluate to 0
   right after initialization).

   @params
   xDim - x dimension size.
   yDim - y dimension size.
   zDim - z dimension size.

   @note
   x varies the fastest in memory, followed by y and z.
-----------------------------------------------------------------------------*/
    patolette__Matrix3D *m = malloc(sizeof(*m));
    m->xDim = xDim;
    m->yDim = yDim;
    m->zDim = zDim;
    m->data = calloc(xDim * yDim * zDim, sizeof(double));
    return m;
}

/*----------------------------------------------------------------------------
   Exported functions END
-----------------------------------------------------------------------------*/