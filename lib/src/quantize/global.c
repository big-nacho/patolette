#include "quantize/global.h"

/*----------------------------------------------------------------------------
   This file defines functions that implement global principal quantization
   as outlined in https://dl.acm.org/doi/pdf/10.1145/146443.146475, albeit
   with some changes.

   Global principal quantization is the first quantization step. From a set
   of colors, a list of color clusters is created by optimizing along the
   color set's principal axis. Later on the resulting clusters will most
   likely be further split in local.c
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
   Constants START
-----------------------------------------------------------------------------*/

static const size_t max_k = 12;
static const double bias_threshold = 0.1;
static const double cell_bias_threshold = 0.9;
static const size_t bucket_count = 512;

/*----------------------------------------------------------------------------
   Constants END
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
   Declarations START
-----------------------------------------------------------------------------*/

static patolette__IndexArray *l_chain(
    const patolette__Matrix2D *L,
    size_t k,
    size_t N
);

static patolette__ColorClusterArray *get_color_clusters(
    const patolette__Matrix2D *colors,
    const patolette__Vector *weights,
    const patolette__IndexArray *quantizer,
    const patolette__IndexArray *bucket_map
);

static patolette__IndexArray *get_principal_quantizer(
    size_t palette_size,
    const patolette__CellMomentsCache *cache
);

static bool should_terminate(
    patolette__IndexArray *quantizer,
    patolette__Vector *axis,
    const patolette__CellMomentsCache *cache,
    bool *error
);

static patolette__IndexArray *get_principal_quantizer(
    size_t palette_size,
    const patolette__CellMomentsCache *cache
);

/*----------------------------------------------------------------------------
   Declarations END
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
   Internal functions START
-----------------------------------------------------------------------------*/

static patolette__IndexArray *l_chain(
    const patolette__Matrix2D *L,
    size_t k, 
    size_t N
) {
/*----------------------------------------------------------------------------
   Utility routine that builds the principal quantizer from the L cache
   used in get_principal_quantizer.

   @params
   L - L cache.
   k - Number of clusters in the principal quantizer.
   N - Upper end of the rightmost cell in the principal quantizer.
-----------------------------------------------------------------------------*/
    patolette__IndexArray *chain = patolette__IndexArray_init(k + 1);
    
    size_t t = N;
    for (size_t j = k - 1; j >= 1; j--) {
        t = patolette__Matrix2D_index(L, j + 1, t);
        patolette__IndexArray_index(chain, j) = t;
    }

    patolette__IndexArray_index(chain, 0) = 0;
    patolette__IndexArray_index(chain, k) = N;
    return chain;
}

static bool should_terminate(
    patolette__IndexArray *quantizer,
    patolette__Vector *axis,
    const patolette__CellMomentsCache *cache,
    bool *error
) {
/*----------------------------------------------------------------------------
   Decides whether the global principal quantization process should
   be terminated.

   This function builds on top of the termination criteria suggested in
   the original paper, i.e. look at every cell of the quantizer and check
   how biased it is against the entire color set's principal axis. If no
   cell has a high enough bias, quantization stops.

   The problem I found with this is that there are datasets (and a
   non-negligible amount too) for which this strategy is not robust enough.
   If some cell in a Q(k) quantizer is biased enough, but contributes little
   enough distortion, it's possible for the Q(k + 1) quantizer to leave
   this cell almost (or outright) untouched, because most of the distortion
   is somewhere else. On the next iteration, the cell will still register a
   high bias, and the quantization will go on for longer than it should.

   This function then uses a similar but slightly different approach.
   We look at each cell in the quantizer and, if it has a high enough
   bias, it then contributes to a global bias. Cells with little distortion
   or biases on the lower end contribute less to the global bias, and vice
   versa.

   This method is not like, rocket science or anything, and it may still
   be broken with unlucky datasets, but in practice it proved to be way
   more robust than the simpler one. There might be a "proper" way to
   circumvent the issue in the first place, but I could not think of it yet.

   @params
   quantizer - The global principal quantizer.
   axis - The principal axis of the color set.
   cache - The CellMomentsCache object.
   error - A flag that is set to true if there's an error.
-----------------------------------------------------------------------------*/
    double distortion = 0;

    for (size_t j = 0; j < quantizer->length - 1; j++) {
        size_t low = patolette__IndexArray_index(quantizer, j);
        size_t high = patolette__IndexArray_index(quantizer, j + 1);

        distortion += patolette__CELLS_get_cell_distortion(
            low,
            high,
            cache
        );
    }

    if (distortion < patolette__DELTA) {
        return true;
    }

    double bias = 0;
    for (size_t i = 0; i < quantizer->length - 1 ; i++) {
        size_t low = patolette__IndexArray_index(quantizer, i);
        size_t high = patolette__IndexArray_index(quantizer, i + 1);

        double cell_distortion = patolette__CELLS_get_cell_distortion(
            low,
            high,
            cache
        );

        double cell_bias = patolette__CELLS_get_cell_bias(
            low,
            high,
            axis,
            cache
        );

        if (cell_bias < 0) {
            *error = true;
            return true;
        }

        if (cell_bias < cell_bias_threshold) {
            continue;
        }

        bias += (cell_distortion / distortion) * cell_bias;
    }

    return bias < bias_threshold;
}

static patolette__IndexArray *get_principal_quantizer(
    size_t palette_size,
    const patolette__CellMomentsCache *cache
) {
/*----------------------------------------------------------------------------
   Gets the global principal quantizer. This is the main function in this
   module. It looks at the CellMomentsCache (created from the bucket-sorted
   color set) and returns the quantizer in the form [0 = q0, q1, ..., qn = N].

   @params
   palette_size - The desired palette size.
   cache - The CellMomentsCache.

   @note
   The desired palette size is typically not reach. The process finishes early,
   creating a lower amount of clusters that are further split in local.c
-----------------------------------------------------------------------------*/
    bool error = false;
    size_t N = cache->size - 1;

    patolette__PCA *pca = patolette__CELLS_perform_PCA(0, N, cache);
    if (pca == NULL) {
        return NULL;
    }


    /*----------------------------------------------------------------------------
         In contrast to the original paper, a copy of E is kept and
         updated every time the outer loop runs. This addresses
         what I believe is a mistake by the author.
    -----------------------------------------------------------------------------*/
    patolette__Vector *E = patolette__Vector_init(N + 1);
    patolette__Vector *E__ = patolette__Vector_init(N + 1);

    size_t L_size = max(palette_size, N);
    patolette__Matrix2D *L = patolette__Matrix2D_init(
        L_size + 1,
        L_size + 1,
        NULL
    );

    for (size_t i = 1; i <= N; i++) {
        patolette__Vector_index(E, i) = patolette__CELLS_get_cell_distortion(
            0,
            i,
            cache
        );
    }

    for (size_t i = 1; i <= palette_size; i++) {
        patolette__Matrix2D_index(L, i, i) = (double)i;
    }

    patolette__IndexArray *result = l_chain(L, 1, N);

    for (size_t k = 2; k <= min(max_k, palette_size); k++) {
        if (
            should_terminate(
                result,
                pca->axis,
                cache,
                &error
            )
        ) {
            break;
        }

        if (error) {
            patolette__Vector_destroy(result);
            result =  NULL;
            break;
        }

        patolette__Vector_copy_into(E, E__);

        /*----------------------------------------------------------------------------
            In the original paper, n goes up to N - palette_size + k,
            but that doesn't account for early termination. Here,
            the full Q(k, N) quantizer is built at each k iteration
            instead, so n goes all the way up to N.
        -----------------------------------------------------------------------------*/
        for (size_t n = k + 1; n <= N; n++) {
            double cut = (double)(n - 1);
            double e = patolette__Vector_index(E__, n - 1);
            
            for (size_t t = n - 2; t >= k - 1; t--) {
                double c = (
                    patolette__Vector_index(E__, t) +
                    patolette__CELLS_get_cell_distortion(t, n, cache)
                );
                
                if (c < e) {
                    cut = (double)t;
                    e = c;
                }
            }

            patolette__Matrix2D_index(L, k, n) = cut;
            patolette__Vector_index(E, n) = e;
        }

        result = l_chain(L, k, N);
    }

    patolette__PCA_destroy(pca);
    patolette__Vector_destroy(E);
    patolette__Vector_destroy(E__);
    patolette__Matrix2D_destroy(L);
    return result;
}

static patolette__ColorClusterArray *get_color_clusters(
    const patolette__Matrix2D *colors,
    const patolette__Vector *weights,
    const patolette__IndexArray *quantizer,
    const patolette__IndexArray *bucket_map
) {
/*----------------------------------------------------------------------------
   Builds color clusters from the principal quantizer.

   @params
   colors - The color set.
   quantizer - The computed global principal quantizer.
   bucket_map - Describes a bucket sorting of the colors based on their
   individual projections onto the color set's principal axis.
-----------------------------------------------------------------------------*/
    size_t count = quantizer->length - 1;

    // Size of each cluster
    patolette__IndexArray *sizes = patolette__IndexArray_init(count);

    // The bucket -> cluster relationship can be cached
    patolette__BoolArray *cache_set = patolette__BoolArray_init(bucket_count);
    patolette__IndexArray *cache = patolette__IndexArray_init(bucket_count);

    for (size_t i = 0; i < bucket_map->length; i++) {
        size_t bucket = patolette__IndexArray_index(bucket_map, i);

        if (!patolette__BoolArray_index(cache_set, bucket)) {
            for (size_t j = 0; j < count; j++) {
                // Quantizer entries use 1-based indexing for buckets, thus bucket + 1
                if (bucket + 1 <= patolette__IndexArray_index(quantizer, j + 1)) {
                    patolette__IndexArray_index(cache, bucket) = j;
                    patolette__BoolArray_index(cache_set, bucket) = true;
                    break;
                }
            }
        }

        size_t k = patolette__IndexArray_index(cache, bucket);
        patolette__IndexArray_index(sizes, k) += 1;
    }

    // The indices of each cluster's colors in the color data set
    patolette__IndexMatrix2D *clusters_indices = patolette__IndexMatrix2D_init(count);
    for (size_t i = 0; i < count; i++) {
        size_t size = patolette__IndexArray_index(sizes, i);
        patolette__IndexMatrix2D_index(clusters_indices, i) = patolette__IndexArray_init(size);
    }

    // Indices arrays are filled incrementally; we store a pivot
    // for each one of them.
    patolette__IndexArray *pivots = patolette__IndexArray_init(count);
    for (size_t i = 0; i < bucket_map->length; i++) {
        size_t bucket = patolette__IndexArray_index(bucket_map, i);
        size_t j = patolette__IndexArray_index(cache, bucket);
        patolette__IndexArray *indices = patolette__IndexMatrix2D_index(clusters_indices, j);
        patolette__IndexArray_index(indices, patolette__IndexArray_index(pivots, j)) = i;
        patolette__IndexArray_index(pivots, j) += 1;
    }

    // Build array of clusters
    patolette__ColorClusterArray *clusters = patolette__ColorClusterArray_init(count);
    for (size_t i = 0; i < count; i++) {
        patolette__IndexArray *indices = patolette__IndexMatrix2D_index(clusters_indices, i);
        patolette__ColorClusterArray_index(clusters, i) = patolette__ColorCluster_init(
            colors,
            weights,
            indices
        );
    }

    patolette__IndexArray_destroy(sizes);
    patolette__BoolArray_destroy(cache_set);
    patolette__IndexArray_destroy(cache);
    patolette__IndexMatrix2D_destroy(clusters_indices);
    patolette__IndexArray_destroy(pivots);
    return clusters;
}

/*----------------------------------------------------------------------------
   Internal functions END
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
   Exported functions START
-----------------------------------------------------------------------------*/

patolette__ColorClusterArray *patolette__GQ_quantize(
    const patolette__Matrix2D *colors,
    const patolette__Vector *weights,
    size_t palette_size
) {
/*----------------------------------------------------------------------------
    Performs global principal quantization.

    @params
    colors - The color set.
    weights - Weight of each color in the color set.
    palette_size - The desired palette size.

    @note
    The desired palette size is typically not reach. The process finishes early,
    creating a lower amount of clusters that are further split in local.c
-----------------------------------------------------------------------------*/
    patolette__ColorClusterArray *result = NULL;

    patolette__PCA *pca = patolette__PCA_perform_PCA(colors, NULL);
    if (pca == NULL) {
        return result;
    }

    patolette__IndexArray *bucket_map = patolette__SORT_axis_sort(
        colors, 
        pca->axis,
        bucket_count
    );
    
    patolette__CellMomentsCache *cache = patolette__CELLS_preprocess(
        colors,
        bucket_map,
        bucket_count
    );

    patolette__IndexArray *quantizer = get_principal_quantizer(
        palette_size,
        cache
    );

    if (quantizer != NULL) {
        result = get_color_clusters(
            colors,
            weights,
            quantizer,
            bucket_map
        );
    }

    patolette__PCA_destroy(pca);
    patolette__IndexArray_destroy(bucket_map);
    patolette__IndexArray_destroy(quantizer);
    patolette__CellMomentsCache_destroy(cache);
    return result;
}

/*----------------------------------------------------------------------------
   Exported functions END
-----------------------------------------------------------------------------*/