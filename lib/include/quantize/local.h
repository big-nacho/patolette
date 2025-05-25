#pragma once

#include <stddef.h>

#include "array/matrix2D.h"

#include "math/pca.h"

#include "quantize/cluster.h"
#include "quantize/sort.h"

patolette__ColorClusterArray *patolette__LQ_quantize(
    patolette__ColorClusterArray *clusters,
    size_t palette_size
);