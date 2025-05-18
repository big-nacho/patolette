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
    patolette__Matrix2D_destroy(cluster->_colors);
    patolette__IndexArray_destroy(cluster->indices);
}

patolette__ColorCluster *patolette__ColorCluster_init(
    const patolette__Matrix2D *dataset,
    patolette__IndexArray *indices
) {
/*----------------------------------------------------------------------------
    Initializes a color cluster.

    @params
    dataset - The color dataset the cluster belongs to.
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
    cluster->dataset__NOTOWNED__ = dataset;
    return cluster;
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

    const patolette__Vector *center = patolette__ColorCluster_get_center(cluster);
    double center_x = patolette__Vector_index(center, 0);
    double center_y = patolette__Vector_index(center, 1);
    double center_z = patolette__Vector_index(center, 2);

    distortion = 0;
    size_t size = cluster->size;
    for (size_t i = 0; i < size; i++) {
        double cx = patolette__Matrix2D_index(colors, i, 0);
        double cy = patolette__Matrix2D_index(colors, i, 1);
        double cz = patolette__Matrix2D_index(colors, i, 2);

        double distance = (
            SQ(cx - center_x) +
            SQ(cy - center_y) +
            SQ(cz - center_z)
        );

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
    center = patolette__Matrix2D_get_vector_mean(colors);
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

    patolette__PCA *pca = patolette__PCA_perform_PCA(colors);
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