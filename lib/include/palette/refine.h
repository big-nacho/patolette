#pragma once

#include "faiss/c_api/Clustering_c.h"

#include "quantize/cluster.h"
#include "quantize/local.h"

patolette__Matrix2D *patolette__PALETTE_get_refined_palette(
    const patolette__Matrix2D *colors,
    const patolette__Vector *weights,
    const patolette__ColorClusterArray *clusters,
    int niter,
    size_t max_samples
);