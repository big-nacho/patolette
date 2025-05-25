#include "quantize/local.h"

/*----------------------------------------------------------------------------
   This file defines functions to turn a set of K color clusters into a set
   of N > K color clusters by greedily optimizing some metric. The splitting
   strategy is as outlined in https://dl.acm.org/doi/pdf/10.1145/146443.146475
   although a weighted variant is implemented.
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
   Constants START
-----------------------------------------------------------------------------*/

static const size_t bucket_count = 512;

/*----------------------------------------------------------------------------
   Constants END
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
   Declarations START
-----------------------------------------------------------------------------*/

typedef struct ClusterPair {
    patolette__ColorCluster *left;
    patolette__ColorCluster *right;
} ClusterPair;

#define ClusterPairArray patolette__Array
#define ClusterPairArray_index(a, i) (patolette__Array_index(ClusterPair*, a, i))
#define ClusterPairArray_init(l) patolette__Array_init(l, sizeof(ClusterPair*))
#define ClusterPairArray_destroy patolette__Array_destroy

static void destroy_cluster_pair(ClusterPair *pair);
static ClusterPair *create_cluster_pair(
    patolette__ColorCluster *left,
    patolette__ColorCluster *right
);

static size_t get_optimal_bucket_index(
    patolette__ColorCluster *cluster,
    patolette__IndexArray *bucket_map
);

static ClusterPair *split_cluster(patolette__ColorCluster *cluster);

static double get_split_benefit(
    patolette__ColorCluster *cluster,
    ClusterPair *children
);

static size_t find_best_cluster_index(
    patolette__ColorClusterArray *clusters,
    ClusterPairArray *children,
    size_t length
);

/*----------------------------------------------------------------------------
   Declarations END
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
   Internal functions START
-----------------------------------------------------------------------------*/

static void destroy_cluster_pair(ClusterPair *pair) {
/*----------------------------------------------------------------------------
   Destroys a color cluster pair.

   @param
   pair - The color cluster pair.
-----------------------------------------------------------------------------*/
    if (pair == NULL) {
        return;
    }

    free(pair->left);
    free(pair->right);
    free(pair);
}

static ClusterPair *create_cluster_pair(
    patolette__ColorCluster *left,
    patolette__ColorCluster *right
) {
/*----------------------------------------------------------------------------
    Creates a color cluster pair.

    @param
    left - The left cluster.
    right - The right cluster.
-----------------------------------------------------------------------------*/
    ClusterPair *pair = malloc(sizeof *pair);
    pair->left = left;
    pair->right = right;
    return pair;
}

static size_t get_optimal_bucket_index(
    patolette__ColorCluster *cluster,
    patolette__IndexArray *bucket_map
) {
/*----------------------------------------------------------------------------
    Gets the optimal bucket index to split a cluster at, based on a
    bucket-sorting of the cluster's colors.

    @param
    cluster - The cluster.
    bucket_map - Describes a bucket-sorting of the colors based on their
    individual projections onto the color set's principal axis.
-----------------------------------------------------------------------------*/
    const patolette__Matrix2D *colors = patolette__ColorCluster_get_colors(cluster);
    const patolette__Vector *weights = patolette__ColorCluster_get_weights(cluster);
    
    // Bucket sizes
    patolette__IndexArray *sizes = patolette__IndexArray_init(bucket_count);

    // Intra-bucket vector sums
    patolette__Matrix2D *sums = patolette__Matrix2D_init(3, bucket_count, NULL);

    for (size_t i = 0; i < bucket_map->length; i++) {
        size_t bucket = patolette__IndexArray_index(bucket_map, i);
        double cx = patolette__Matrix2D_index(colors, i, 0);
        double cy = patolette__Matrix2D_index(colors, i, 1);
        double cz = patolette__Matrix2D_index(colors, i, 2);
        double weight = weights == NULL ? 1 : patolette__Vector_index(weights, i);
        patolette__Matrix2D_index(sums, 0, bucket) += cx * weight;
        patolette__Matrix2D_index(sums, 1, bucket) += cy * weight;
        patolette__Matrix2D_index(sums, 2, bucket) += cz * weight;
        patolette__IndexArray_index(sizes, bucket) += weight;
    }

    // Intra-bucket vector sums are made cumulative
    for (size_t i = 1; i < bucket_count; i++) {
        patolette__Matrix2D_index(sums, 0, i) += patolette__Matrix2D_index(sums, 0, i - 1);
        patolette__Matrix2D_index(sums, 1, i) += patolette__Matrix2D_index(sums, 1, i - 1);
        patolette__Matrix2D_index(sums, 2, i) += patolette__Matrix2D_index(sums, 2, i - 1);
    }

    // Bucket sizes are made cumulative
    for (size_t i = 1; i < bucket_count; i++) {
        patolette__IndexArray_index(sizes, i) += patolette__IndexArray_index(sizes, i - 1);
    }

    // Objective function
    patolette__Vector *objective = patolette__Vector_init(bucket_count);
    for (size_t i = 0; i < bucket_count; i++) {
        for (size_t j = 0; j < 3; j++) {
            double csl = patolette__Matrix2D_index(sums, j, i);
            double csr = patolette__Matrix2D_index(sums, j, bucket_count - 1) - csl;
            double sl = patolette__IndexArray_index(sizes, i);
            double sr = patolette__IndexArray_index(sizes, bucket_count - 1) - sl;

            double v = 0;
            if (sl != 0) {
                v += SQ(csl) / sl;
            }

            if (sr != 0) {
                v += SQ(csr) / sr;
            }

            patolette__Vector_index(objective, i) += v;
        }
    }

    // We want to maximize the objective function
    size_t result = patolette__Vector_maxloc(objective);

    patolette__Matrix2D_destroy(sums);
    patolette__IndexArray_destroy(sizes);
    patolette__Vector_destroy(objective);
    return result;
}

static ClusterPair *split_cluster(patolette__ColorCluster *cluster) {
/*----------------------------------------------------------------------------
    Splits a cluster.

    @param
    cluster - The cluster to split.
-----------------------------------------------------------------------------*/
    size_t size = cluster->size;
    if (size <= 1) {
        // Can't be split
        return NULL;
    }

    const patolette__Matrix2D *colors = patolette__ColorCluster_get_colors(cluster);
    const patolette__Vector *axis = patolette__ColorCluster_get_principal_axis(cluster);

    if (axis == NULL) {
        return NULL;
    }

    patolette__IndexArray *bucket_map = patolette__SORT_axis_sort(
        colors,
        axis,
        bucket_count
    );

    size_t split_index = get_optimal_bucket_index(
        cluster,
        bucket_map
    );

    // Left cluster size
    size_t left_size = 0;

    // Right cluster size
    size_t right_size = 0;

    for (size_t i = 0; i < bucket_map->length; i++) {
        if (patolette__IndexArray_index(bucket_map, i) <= split_index) {
            left_size++;
        }
        else {
            right_size++;
        }
    }

    const patolette__IndexArray *indices = cluster->indices;
    patolette__IndexArray *left_indices = patolette__IndexArray_init(left_size);
    patolette__IndexArray *right_indices = patolette__IndexArray_init(right_size);

    // Two pivots to populate each cluster incrementally
    size_t pivot_left = 0;
    size_t pivot_right = 0;

    for (size_t i = 0; i < bucket_map->length; i++) {
        size_t index = patolette__IndexArray_index(indices, i);
        if (patolette__IndexArray_index(bucket_map, i) <= split_index) {
            patolette__IndexArray_index(left_indices, pivot_left) = index;
            pivot_left++;
        }
        else {
            patolette__IndexArray_index(right_indices, pivot_right) = index;
            pivot_right++;
        }
    }

    const patolette__Matrix2D *dataset = cluster->dataset__NOTOWNED__;
    const patolette__Vector *dataset_weights = cluster->dataset_weights__NOTOWNED__;
    ClusterPair *children = create_cluster_pair(
        patolette__ColorCluster_init(dataset, dataset_weights, left_indices),
        patolette__ColorCluster_init(dataset, dataset_weights, right_indices)
    );

    patolette__IndexArray_destroy(bucket_map);
    return children;
}

static double get_split_benefit(
    patolette__ColorCluster *cluster,
    ClusterPair *children
) {
/*----------------------------------------------------------------------------
    Gets the benefit of splitting a cluster.

    @param
    cluster - The cluster.
    children - The cluster's children.
-----------------------------------------------------------------------------*/
    if (children == NULL) {
        return 0;
    }

    double d = patolette__ColorCluster_get_distortion(cluster);
    double dl = patolette__ColorCluster_get_distortion(children->left);
    double dr = patolette__ColorCluster_get_distortion(children->right);
    return d - (dl + dr);
}

static size_t find_best_cluster_index(
    patolette__ColorClusterArray *clusters,
    ClusterPairArray *children,
    size_t length
) {
/*----------------------------------------------------------------------------
    Finds the index of the best cluster to split.

    @param
    clusters - The list of clusters.
    children - A list containing each cluster's children.
    length - *Defined* portion of the clusters array (can have padding to the right).
-----------------------------------------------------------------------------*/
    patolette__Vector *benefits = patolette__Vector_init(length);

    for (size_t i = 0; i < length; i++) {
        patolette__ColorCluster *cluster = patolette__ColorClusterArray_index(clusters, i);
        ClusterPair *cluster_children = ClusterPairArray_index(children, i);

        if (cluster_children == NULL) {
            patolette__Vector_index(benefits, i) = 0;
            continue;
        }

        patolette__Vector_index(benefits, i) = get_split_benefit(cluster, cluster_children);
    }

    size_t result = patolette__Vector_maxloc(benefits);
    patolette__Vector_destroy(benefits);
    return result;
}

/*----------------------------------------------------------------------------
    Internal functions END
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
    Exported functions START
-----------------------------------------------------------------------------*/

patolette__ColorClusterArray *patolette__LQ_quantize(
    patolette__ColorClusterArray *clusters,
    size_t palette_size
) {
/*----------------------------------------------------------------------------
    Splits a set of K color clusters into N > K color clusters.

    @params
    clusters - The initial list of clusters.
    palette_size - The desired palette size (N).
-----------------------------------------------------------------------------*/
    if (clusters->length >= palette_size) {
        return clusters;
    }

    patolette__ColorClusterArray *result = patolette__ColorClusterArray_init(palette_size);
    patolette__ColorClusterArray_copy_into(clusters, result);

    ClusterPairArray *children = ClusterPairArray_init(palette_size);
    for (size_t i = 0; i < clusters->length; i++) {
        patolette__ColorCluster *cluster = patolette__ColorClusterArray_index(clusters, i);
        ClusterPairArray_index(children, i) = split_cluster(cluster);
    }

    for (size_t i = clusters->length; i < palette_size; i++) {
        size_t best_cluster_index = find_best_cluster_index(
            result,
            children,
            i
        );

        patolette__ColorCluster *best_cluster = patolette__ColorClusterArray_index(
            result,
            best_cluster_index
        );

        ClusterPair *best_cluster_children = ClusterPairArray_index(
            children,
            best_cluster_index
        );

        double benefit = get_split_benefit(best_cluster, best_cluster_children);
        if (benefit < patolette__DELTA) {
            patolette__ColorClusterArray *slice = patolette__ColorClusterArray_slice(result, 0, i);
            patolette__ColorClusterArray_destroy(result);
            result = slice;
            break;
        }

        patolette__ColorCluster *left = best_cluster_children->left;
        patolette__ColorCluster *right = best_cluster_children->right;

        patolette__ColorClusterArray_index(result, i) = left;
        patolette__ColorClusterArray_index(result, best_cluster_index) = right;

        ClusterPairArray_index(children, i) = split_cluster(left);
        ClusterPairArray_index(children, best_cluster_index) = split_cluster(right);

        patolette__ColorCluster_destroy(best_cluster);
    }

    for (size_t i = 0; i < children->length; i++) {
        ClusterPair *pair = ClusterPairArray_index(children, i);
        destroy_cluster_pair(pair);
    }

    ClusterPairArray_destroy(children);
    return result;
}

/*----------------------------------------------------------------------------
    Exported functions END
-----------------------------------------------------------------------------*/