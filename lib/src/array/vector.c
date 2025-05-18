#include "array/vector.h"

/*----------------------------------------------------------------------------
   patolette__Vector

   This file defines functions that work on vectors. The actual
   patolette__Vector definition can be found in "array/vector.h".
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
   Declarations START
-----------------------------------------------------------------------------*/

static size_t patolette__Vector_minmaxloc(const patolette__Vector *v, bool comp_max);

/*----------------------------------------------------------------------------
   Declarations END
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
   Internal functions START
-----------------------------------------------------------------------------*/

static size_t patolette__Vector_minmaxloc(const patolette__Vector *v, bool comp_max) {
/*----------------------------------------------------------------------------
   Finds the location (index) of the minimum, or maximum of a Vector.

   @params
   v - The Vector.
   comp_max - If true, the maximum is looked for, otherwise the minimum.
-----------------------------------------------------------------------------*/
    size_t loc = 0;
    double best = patolette__Vector_index(v, loc);
    for (size_t i = 0; i < v->length; i++) {
        double val = patolette__Vector_index(v, i);
        bool better = comp_max ? val > best : val < best;
        if (better) {
            best = val;
            loc = i;
        }
    }

    return loc;
}

/*----------------------------------------------------------------------------
   Internal functions END
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
   Exported functions START
-----------------------------------------------------------------------------*/

size_t patolette__Vector_minloc(const patolette__Vector *v) {
/*----------------------------------------------------------------------------
   Finds the location (index) of the minimum of a Vector.

   @params
   v - The Vector.
-----------------------------------------------------------------------------*/
    return patolette__Vector_minmaxloc(v, false);
}

size_t patolette__Vector_maxloc(const patolette__Vector *v) {
/*----------------------------------------------------------------------------
   Finds the location (index) of the maximum of a Vector.

   @params
   v - The Vector.
-----------------------------------------------------------------------------*/
    return patolette__Vector_minmaxloc(v, true);
}

double patolette__Vector_min(const patolette__Vector *v) {
/*----------------------------------------------------------------------------
   Finds the minimum of a Vector.

   @params
   v - The Vector.
-----------------------------------------------------------------------------*/
    return patolette__Vector_index(v, patolette__Vector_minloc(v));
}

double patolette__Vector_max(const patolette__Vector *v) {
/*----------------------------------------------------------------------------
   Finds the maximum of a Vector.

   @params
   v - The Vector.
-----------------------------------------------------------------------------*/
    return patolette__Vector_index(v, patolette__Vector_maxloc(v));
}

double patolette__Vector_sum(const patolette__Vector *v) {
/*----------------------------------------------------------------------------
   Computes the sum of a vector's elements.

   @params
   v - The Vector.
-----------------------------------------------------------------------------*/
    double s = 0;
    for (size_t i = 0; i < v->length; i++) {
        s += patolette__Vector_index(v, i);
    }
    return s;
}

void patolette__Vector_scale(const patolette__Vector *v, double s) {
/*----------------------------------------------------------------------------
   Scales a vector (the input vector is modified).

   @params
   v - The Vector.
   s - The scale factor.
-----------------------------------------------------------------------------*/
    for (size_t i = 0; i < v->length; i++) {
        patolette__Vector_index(v, i) *= s;
    }
}

double patolette__Vector_snorm(const patolette__Vector *v) {
/*----------------------------------------------------------------------------
   Returns the squared norm (length) of a vector.

   @params
   v - The Vector.
-----------------------------------------------------------------------------*/
    double s = 0;
    for (size_t i = 0; i < v->length; i++) {
        s += pow(patolette__Vector_index(v, i), 2);
    }
    return s;
}

double patolette__Vector_norm(const patolette__Vector *v) {
/*----------------------------------------------------------------------------
   Returns the norm (length) of a vector.

   @params
   v - The Vector.
-----------------------------------------------------------------------------*/
    return sqrt(patolette__Vector_snorm(v));
}

// DEBUG
void patolette__Vector_print(patolette__Vector *v) {
    size_t length = v->length;
    for (size_t i = 0; i < length; i++) {
        printf("%f", patolette__Vector_index(v, i));
        printf("\t");
    }
    printf("\n");
}

/*----------------------------------------------------------------------------
   Exported functions END
-----------------------------------------------------------------------------*/