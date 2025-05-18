#pragma once

#include "math/misc.h"
#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "array/array.h"

/*----------------------------------------------------------------------------
   patolette__Vector

   A fancy array of doubles. Sometimes used as generic storage and
   sometimes as an actual direction-magnitude vector.
-----------------------------------------------------------------------------*/

#define patolette__Vector patolette__Array
#define patolette__Vector_index(a, i) (patolette__Array_index(double, a, i))
#define patolette__Vector_destroy patolette__Array_destroy
#define patolette__Vector_init(l) patolette__Array_init(l, sizeof(double))
#define patolette__Vector_copy_into patolette__Array_copy_into

size_t patolette__Vector_minloc(const patolette__Vector *arr);
size_t patolette__Vector_maxloc(const patolette__Vector *arr);
double patolette__Vector_min(const patolette__Vector *arr);
double patolette__Vector_max(const patolette__Vector *arr);
double patolette__Vector_sum(const patolette__Vector *v);
void patolette__Vector_scale(const patolette__Vector *v, double s);
double patolette__Vector_snorm(const patolette__Vector *v);
double patolette__Vector_norm(const patolette__Vector *v);

// DEBUG
void patolette__Vector_print(patolette__Vector *v);