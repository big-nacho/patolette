#include "quantize/cluster.h"

/*----------------------------------------------------------------------------
    patolette__ColorCluster

    This file defines functions that work on color clusters. The actual
    patolette__ColorClusterArray definition can be found in "quantize/cluster.h"

    The main properties of interest:
    1. Distortion (sum of squared deviations, or size-weighted variance)
    2. Principal axis
    3. Colors
    4. Center
    5. Weights

    Should only be retrieved via their respective getters. They are all
    computed only once and then cached.

    @note
    Cluster splitting is performed in local.c
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
    Exported functions START
-----------------------------------------------------------------------------*/

void patolette__ColorCluster_destroy(patolette__ColorCluster *cluster) {
/*----------------------------------------------------------------------------
    Destroys a color cluster.

    @params
    cluster - The cluster to destroy.
-----------------------------------------------------------------------------*/

    if (cluster == NULL) {
        return;
    }

    patolette__Vector_destroy(cluster->_center);
    patolette__Vector_destroy(cluster->_principal_axis);
    patolette__Vector_destroy(cluster->weights);
    patolette__Matrix2D_destroy(cluster->_colors);
    patolette__IndexArray_destroy(cluster->indices);
    free(cluster);
}

patolette__ColorCluster *patolette__ColorCluster_init(
    const patolette__Matrix2D *dataset,
    const patolette__Vector *dataset_weights,
    patolette__IndexArray *indices
) {
/*----------------------------------------------------------------------------
    Initializes a color cluster.

    @params
    dataset - The color dataset the cluster belongs to.
    dataset_weights - Weight of each sample in the dataset.
    indices - The indices of the cluster's colors in the color dataset.
-----------------------------------------------------------------------------*/
    patolette__ColorCluster *cluster = malloc(sizeof *cluster);

    // All these properties start undefined.
    // They're only computed when first requested via
    // their getters, and cached for later use.
    cluster->_distortion = -1.0;
    cluster->_principal_axis = NULL;
    cluster->_colors = NULL;
    cluster->_center = NULL;

    cluster->size = indices->length;
    cluster->indices = indices;
    cluster->weights = NULL;
    cluster->dataset__NOTOWNED__ = dataset;

    cluster->dataset_weights__NOTOWNED__ = dataset_weights;
    return cluster;
}

const patolette__Vector *patolette__ColorCluster_get_weights(patolette__ColorCluster *cluster) {
/*----------------------------------------------------------------------------
    Gets the weight of each one of the color cluster's colors.

    @params
    cluster - The color cluster.
-----------------------------------------------------------------------------*/
    const patolette__Vector *dataset_weights = cluster->dataset_weights__NOTOWNED__;
    if (dataset_weights == NULL) {
        return NULL;
    }

    patolette__Vector *weights = cluster->weights;

    if (weights != NULL) {
        // Weights are cached
        return weights;
    }

    weights = patolette__Vector_init(cluster->size);

    const patolette__IndexArray *indices = cluster->indices;
    for (size_t i = 0; i < indices->length; i++) {
        size_t index = patolette__IndexArray_index(indices, i);
        patolette__Vector_index(weights, i) = patolette__Vector_index(dataset_weights, index);
    }

    cluster->weights = weights;
    return weights;
}

double patolette__ColorCluster_get_distortion(patolette__ColorCluster *cluster) {
/*----------------------------------------------------------------------------
    Gets a color cluster's distortion.

    @params
    cluster - The color cluster.
-----------------------------------------------------------------------------*/
    double distortion = cluster->_distortion;

    if (distortion != -1.0) {
        // Distortion is cached
        return distortion;
    }

    const patolette__Matrix2D *colors = patolette__ColorCluster_get_colors(cluster);
    const patolette__Vector *weights = patolette__ColorCluster_get_weights(cluster);

    const patolette__Vector *center = patolette__ColorCluster_get_center(cluster);
    double x = patolette__Vector_index(center, 0);
    double y = patolette__Vector_index(center, 1);
    double z = patolette__Vector_index(center, 2);

    distortion = 0;
    size_t size = cluster->size;
    for (size_t i = 0; i < size; i++) {
        double weight = weights == NULL ? 1 : patolette__Vector_index(weights, i);
        double cx = patolette__Matrix2D_index(colors, i, 0);
        double cy = patolette__Matrix2D_index(colors, i, 1);
        double cz = patolette__Matrix2D_index(colors, i, 2);

        double distance = (
          SQ(cx - x) +
          SQ(cy - y) +
          SQ(cz - z)
        ) * weight;

        distortion += distance;
    }

    cluster->_distortion = distortion;
    return distortion;
}

double patolette__ColorCluster_get_variance(patolette__ColorCluster *cluster) {
/*----------------------------------------------------------------------------
    Gets a color cluster's variance.

    @params
    cluster - The color cluster.
-----------------------------------------------------------------------------*/
    size_t size = cluster->size;

    if (size == 0) {
        return 0;
    }

    double distortion = patolette__ColorCluster_get_distortion(cluster);
    return distortion / (double)size;
}

const patolette__Vector *patolette__ColorCluster_get_center(patolette__ColorCluster *cluster) {
/*----------------------------------------------------------------------------
    Gets a color cluster's center (mean).

    @params
    cluster - The color cluster.
-----------------------------------------------------------------------------*/
    patolette__Vector *center = cluster->_center;

    if (center != NULL) {
        return center;
    }

    const patolette__Matrix2D *colors = patolette__ColorCluster_get_colors(cluster);
    const patolette__Vector *weights = patolette__ColorCluster_get_weights(cluster);
    center = patolette__Matrix2D_get_vector_mean(colors, weights);
    cluster->_center = center;
    return center;
}

const patolette__Vector *patolette__ColorCluster_get_principal_axis(patolette__ColorCluster *cluster) {
/*----------------------------------------------------------------------------
    Gets a color cluster's principal axis.

    @params
    cluster - The color cluster.
-----------------------------------------------------------------------------*/
    patolette__Vector *axis = cluster->_principal_axis;

    if (axis != NULL) {
        // Cluster's principal axis is cached
        return axis;
    }

    const patolette__Matrix2D *colors = patolette__ColorCluster_get_colors(cluster);
    const patolette__Vector *weights = patolette__ColorCluster_get_weights(cluster);

    patolette__PCA *pca = patolette__PCA_perform_PCA(colors, weights);
    if (pca != NULL) {
        axis = patolette__Vector_init(pca->axis->length);
        patolette__Vector_copy_into(pca->axis, axis);
        cluster->_principal_axis = axis;
    }

    patolette__PCA_destroy(pca);
    return axis;
}

const patolette__Matrix2D *patolette__ColorCluster_get_colors(patolette__ColorCluster *cluster) {
/*----------------------------------------------------------------------------
    Gets a color cluster's colors.

    @params
    cluster - The color cluster.
-----------------------------------------------------------------------------*/
    patolette__Matrix2D *colors = cluster->_colors;

    if (colors != NULL) {
        // Cluster colors are cached
        return colors;
    }

    const patolette__Matrix2D  *dataset = cluster->dataset__NOTOWNED__;
    const patolette__IndexArray *indices = cluster->indices;
    colors = patolette__Matrix2D_extract_rows(dataset, indices);
    cluster->_colors = colors;
    return colors;
}

/*----------------------------------------------------------------------------
    Exported functions START
-----------------------------------------------------------------------------*/