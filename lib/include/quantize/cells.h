#pragma once

#include <stddef.h>

#include "array/array.h"
#include "array/matrix2D.h"
#include "array/matrix3D.h"
#include "array/vector.h"

#include "math/eigen.h"
#include "math/pca.h"

typedef struct patolette__CellMomentsCache {
    patolette__UInt64Array *w0;
    patolette__Matrix2D *w1;
    patolette__Vector *w2;
    patolette__Matrix3D *wrs;
    size_t size;
} patolette__CellMomentsCache;

void patolette__CellMomentsCache_destroy(patolette__CellMomentsCache *cache);

patolette__CellMomentsCache *patolette__CELLS_preprocess(
    const patolette__Matrix2D *colors,
    const patolette__IndexArray *bucket_map,
    size_t bucket_count
);

double patolette__CELLS_get_cell_distortion(
    size_t a, 
    size_t b, 
    const patolette__CellMomentsCache *cache
);

patolette__PCA *patolette__CELLS_perform_PCA(
    size_t a, 
    size_t b, 
    const patolette__CellMomentsCache *cache
);

double patolette__CELLS_get_cell_bias(
    size_t a,
    size_t b,
    const patolette__Vector *axis,
    const patolette__CellMomentsCache *cache
);