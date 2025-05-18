#pragma once

#include "flann/flann.h"

#include "array/matrix2D.h"

flann_index_t patolette__PALETTE_build_palette_index(
    const patolette__Matrix2D *colors,
    double fx,
    double fy,
    double fz,
    struct FLANNParameters *params
);

size_t patolette__PALETTE_find_closest(
    double x,
    double y,
    double z,
    flann_index_t index,
    struct FLANNParameters *params
);

void patolette__PALETTE_fill_palette_map_nearest(
    const patolette__Matrix2D *colors,
    const patolette__Matrix2D *palette_colors,
    size_t *palette_map
);

patolette__Vector *patolette__PALETTE_get_knn_total_distances(
    const patolette__Matrix2D *colors,
    size_t k
);