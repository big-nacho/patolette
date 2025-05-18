#pragma once

#include <stdbool.h>
#include <stdio.h>

#include "array/array.h"
#include "array/matrix2D.h"
#include "array/vector.h"

#include "math/pca.h"

#include "quantize/sort.h"
#include "quantize/cluster.h"
#include "quantize/cells.h"

patolette__ColorClusterArray *patolette__GQ_quantize(
    const patolette__Matrix2D *colors,
    size_t palette_size
);