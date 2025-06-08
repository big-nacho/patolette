#include "palette/refine.h"

/*----------------------------------------------------------------------------
    This file defines functions to perform color palette refinement via
    KMeans iteration.

    KMeans is run via FAISS.
    FAISS: https://github.com/facebookresearch/faiss

    @note
    We need a version of FAISS with a slightly modified C API (the standard
    one doesn't provide a way to set initial centers or weights easily).
    That's why the FAISS source code is included with this library, at least
    for now ¯\_(ツ)_/¯
-----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
    Constants START
-----------------------------------------------------------------------------*/

size_t min_kmeans_samples = SQ(256);

/*----------------------------------------------------------------------------
    Constants END
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
    Declarations START
-----------------------------------------------------------------------------*/

static void kmeans(
    float *centers,
    float *samples,
    float *weights,
    size_t center_count,
    size_t sample_count,
    int niter,
    size_t max_samples
);

static float *get_centers(const patolette__ColorClusterArray *clusters);
static float *get_samples(const patolette__Matrix2D *colors);
static float *get_weights(const patolette__Vector *weights);

/*----------------------------------------------------------------------------
    Declarations END
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
    Exported functions START
-----------------------------------------------------------------------------*/

static void kmeans(
    float *centers,
    float *samples,
    float *weights,
    size_t center_count,
    size_t sample_count,
    int niter,
    size_t max_samples
) {
/*----------------------------------------------------------------------------
    Runs KMeans.

    @params
    centers - List of initial centers.
    samples - List of samples.
    center_count - Number of centers (cluster count).
    sample_count - Number of samples.
    niter - Number of iterations.
    max_samples - Maximum number of samples to use.
-----------------------------------------------------------------------------*/
    FaissClusteringParameters params;
    faiss_ClusteringParameters_init(&params);
    params.niter = niter;
    params.nredo = 1;
    params.verbose = false;
    params.spherical = false;
    params.int_centroids = false;
    params.update_index = false;
    params.frozen_centroids = false;
    params.min_points_per_centroid = 1;
    params.max_points_per_centroid = (int)(max(max_samples, min_kmeans_samples) / center_count);
    params.seed = 1234;
    params.decode_block_size = 32768;

    faiss_kmeans_clustering(
        3,
        sample_count,
        center_count,
        samples,
        centers,
        weights,
        &params
    );
}

static float *get_centers(const patolette__ColorClusterArray *clusters) {
/*----------------------------------------------------------------------------
    Gets initial centers data for FAISS.

    @params
    clusters - List of initial clusters.
-----------------------------------------------------------------------------*/
    float *centers = malloc(sizeof(float) * clusters->length * 3);

    for (size_t i = 0; i < clusters->length; i++) {
        patolette__ColorCluster *cluster = patolette__ColorClusterArray_index(clusters, i);
        const patolette__Vector *center = patolette__ColorCluster_get_center(cluster);

        double cx = patolette__Vector_index(center, 0);
        double cy = patolette__Vector_index(center, 1);
        double cz = patolette__Vector_index(center, 2);

        centers[i * 3] = (float)cx;
        centers[i * 3 + 1] = (float)cy;
        centers[i * 3 + 2] = (float)cz;
    }

    return centers;
}

static float *get_samples(const patolette__Matrix2D *colors) {
/*----------------------------------------------------------------------------
    Gets samples data for FAISS.

    @params
    colors - List of color samples.
-----------------------------------------------------------------------------*/
    float *samples = malloc(sizeof(float) * colors->rows * 3);

    for (size_t i = 0; i < colors->rows; i++) {
        double cx = patolette__Matrix2D_index(colors, i, 0);
        double cy = patolette__Matrix2D_index(colors, i, 1);
        double cz = patolette__Matrix2D_index(colors, i, 2);
        samples[i * 3] = (float)cx;
        samples[i * 3 + 1] = (float)cy;
        samples[i * 3 + 2] = (float)cz;
    }

    return samples;
}

static float *get_weights(const patolette__Vector *weights) {
/*----------------------------------------------------------------------------
    Gets weights data for FAISS.

    @params
    colors - List of color weights.
-----------------------------------------------------------------------------*/
    float *fweights = malloc(sizeof(float) * weights->length);

    for (size_t i = 0; i < weights->length; i++) {
        double w = patolette__Vector_index(weights, i);
        fweights[i] = (float)w;
    }

    return fweights;
}

patolette__Matrix2D *patolette__PALETTE_get_refined_palette(
    const patolette__Matrix2D *colors,
    const patolette__Vector *weights,
    const patolette__ColorClusterArray *clusters,
    int niter,
    size_t max_samples
) {
/*----------------------------------------------------------------------------
    Refines a color palette via KMeans iteration.

    @params
    colors - List of colors to quantize.
    clusters - List of clusters resulting from an earlier quantization.
    niter - Number of KMeans iterations.
    max_samples - Maximum number of samples to use.
-----------------------------------------------------------------------------*/

    float *samples = get_samples(colors);
    float *centers = get_centers(clusters);

    float *fweights = NULL;
    if (weights != NULL) {
        fweights = get_weights(weights);
    }

    kmeans(
        centers,
        samples,
        fweights,
        clusters->length,
        colors->rows,
        niter,
        max_samples
    );

    patolette__Matrix2D *palette = patolette__Matrix2D_init(
        clusters->length,
        3,
        NULL
    );

    for (size_t i = 0; i < clusters->length; i++) {
        patolette__Matrix2D_index(palette, i, 0) = (double)centers[i * 3];
        patolette__Matrix2D_index(palette, i, 1) = (double)centers[i * 3 + 1];
        patolette__Matrix2D_index(palette, i, 2) = (double)centers[i * 3 + 2];
    }

    free(centers);
    free(samples);
    if (fweights != NULL) {
        free(fweights);
    }

    return palette;
}

/*----------------------------------------------------------------------------
    Exported functions END
-----------------------------------------------------------------------------*/