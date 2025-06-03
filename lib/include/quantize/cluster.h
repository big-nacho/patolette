#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "array/array.h"

#include "array/matrix2D.h"
#include "math/pca.h"

/*----------------------------------------------------------------------------
    patolette__ColorCluster

    A simple cluster of colors.
-----------------------------------------------------------------------------*/

// Macros for color cluster arrays
#define patolette__ColorClusterArray patolette__Array
#define patolette__ColorClusterArray_index(a, i) (patolette__Array_index(patolette__ColorCluster*, a, i))
#define patolette__ColorClusterArray_init(l) patolette__Array_init(l, sizeof(patolette__ColorCluster*))
#define patolette__ColorClusterArray_copy_into patolette__Array_copy_into
#define patolette__ColorClusterArray_slice patolette__Array_slice
#define patolette__ColorClusterArray_destroy patolette__Array_destroy

typedef struct patolette__ColorCluster patolette__ColorCluster;
struct patolette__ColorCluster {
    // The cluster's distortion
    double _distortion;

    // The cluster's principal axis
    patolette__Vector *_principal_axis;

    // The cluster's colors
    patolette__Matrix2D *_colors;

    // The cluster's center (mean)
    patolette__Vector *_center;

    /*----------------------------------------------------------------------------
        The indices of the cluster's colors in the dataset.

        @example
        If dataset__NOTOWNED__ = | 1 2 3 |
                                 | 3 0 0 |
                                 | 2 2 2 |

           _indices = | 0, 1 | then

           _colors = | 1 2 3 |
                     | 3 0 0 |
    -----------------------------------------------------------------------------*/
    patolette__IndexArray *indices;

    // The weight of each color
    patolette__Vector  *weights;

    // Size of the cluster, matches the length of indices
    size_t size;

    // The dataset the cluster belongs to
    // This property is not owned by the cluster, and so
    // it must be destroyed separately
    const patolette__Matrix2D  *dataset__NOTOWNED__;

    // The weights of each sample in the dataset
    // This property is not owned by the cluster, and so
    // it must be destroyed separately
    const patolette__Vector  *dataset_weights__NOTOWNED__;
};

void patolette__ColorCluster_destroy(patolette__ColorCluster *cluster);
patolette__ColorCluster *patolette__ColorCluster_init(
    const patolette__Matrix2D *dataset,
    const patolette__Vector *weights,
    patolette__IndexArray *indices
);

const patolette__Vector *patolette__ColorCluster_get_weights(patolette__ColorCluster *cluster);
double patolette__ColorCluster_get_distortion(patolette__ColorCluster *cluster);
double patolette__ColorCluster_get_variance(patolette__ColorCluster *cluster);
const patolette__Vector *patolette__ColorCluster_get_center(patolette__ColorCluster *cluster);
const patolette__Vector *patolette__ColorCluster_get_principal_axis(patolette__ColorCluster *cluster);
const patolette__Matrix2D *patolette__ColorCluster_get_colors(patolette__ColorCluster *cluster);

// TODO: refactor, weird placement
void patolette__ColorClusterArray_destroy_deep(patolette__ColorClusterArray *array);