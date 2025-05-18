#pragma once

#include <stdlib.h>
#include <stddef.h>

#include "array/array.h"
#include "array/matrix2D.h"
#include "array/vector.h"

#include "math/linalg.h"
#include "math/misc.h"

patolette__IndexArray *patolette__SORT_axis_sort(
    const patolette__Matrix2D *colors,
    const patolette__Vector *axis,
    size_t bucket_count
);