#include "quantize/cells.h"

/*----------------------------------------------------------------------------
    This file defines functions to perform queries on the cells of the
    global principal quantizer, as well as to preprocess the cumulative
    moments needed to perform such queries.

    For more context and detail about the maths, Wu's original paper:
    https://dl.acm.org/doi/pdf/10.1145/146443.146475
-----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
    Declarations START
-----------------------------------------------------------------------------*/

static double eval_vcov(
    size_t a,
    size_t b,
    size_t r,
    size_t s,
    const patolette__CellMomentsCache *cache
);

static patolette__Matrix2D *patolette__CELLS_vcov(
    size_t a,
    size_t b,
    const patolette__CellMomentsCache *cache
);

/*----------------------------------------------------------------------------
    Declarations END
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
    Internal functions START
-----------------------------------------------------------------------------*/

void patolette__CellMomentsCache_destroy(patolette__CellMomentsCache *cache) {
/*----------------------------------------------------------------------------
    Destroys a CellMomentsCache object

    @params
    cache - The CellMomentsCache object.
-----------------------------------------------------------------------------*/
    patolette__UInt64Array_destroy(cache->w0);
    patolette__Matrix2D_destroy(cache->w1);
    patolette__Vector_destroy(cache->w2);
    patolette__Matrix3D_destroy(cache->wrs);
    free(cache);
}

patolette__CellMomentsCache *patolette__CELLS_preprocess(
    const patolette__Matrix2D *colors,
    const patolette__IndexArray *bucket_map,
    size_t bucket_count
) {
/*----------------------------------------------------------------------------
    Constructs the CellMomentsCache object needed to perform queries
    on the cells of the global principal quantizer.

    @params
    colors - A list of colors.
    bucket_map - Describes a bucket sorting of the colors based on their
    individual projections onto the color set's principal axis.
    bucket_count - The number of buckets used to sort the colors.

    @note
    For queries (0, k] to work as intended, we use 1-based indexing
    for the bucket map.
-----------------------------------------------------------------------------*/
    patolette__CellMomentsCache *cache = malloc(sizeof(*cache));

    // Check @note for why bucket_count + 1 is used
    size_t size = bucket_count + 1;
    cache->w0 = patolette__UInt64Array_init(size);
    cache->w1 = patolette__Matrix2D_init(3, size, NULL);
    cache->w2 = patolette__Vector_init(size);
    cache->wrs = patolette__Matrix3D_init(3, 3, size);
    cache->size = size;

    patolette__UInt64Array *w0 = cache->w0;
    patolette__Matrix2D *w1 = cache->w1;
    patolette__Vector *w2 = cache->w2;
    patolette__Matrix3D *wrs = cache->wrs;

    size_t rows = colors->rows;
    for (size_t i = 0; i < rows; i++) {
        size_t j = patolette__IndexArray_index(bucket_map, i) + 1;

        double cx = patolette__Matrix2D_index(colors, i, 0);
        double cy = patolette__Matrix2D_index(colors, i, 1);
        double cz = patolette__Matrix2D_index(colors, i, 2);

        patolette__UInt64Array_index(w0, j) += 1;
        patolette__Matrix2D_index(w1, 0, j) += cx;
        patolette__Matrix2D_index(w1, 1, j) += cy;
        patolette__Matrix2D_index(w1, 2, j) += cz;
        patolette__Vector_index(w2, j) += (
            SQ(cx) +
            SQ(cy) +
            SQ(cz)
        );
    }

    for (size_t i = 0; i < rows; i++) {
        size_t j = patolette__IndexArray_index(bucket_map, i) + 1;

        for (size_t s = 0; s < 3; s++) {
            for (size_t r = 0; r <= s; r++) {
                double cr = patolette__Matrix2D_index(colors, i, r);
                double cs = patolette__Matrix2D_index(colors, i, s);
                patolette__Matrix3D_index(wrs, r, s, j) += cr * cs;
            }
        }
    }

    for (size_t i = 1; i < size; i++) {
        patolette__UInt64Array_index(w0, i) += patolette__UInt64Array_index(w0, i - 1);
        patolette__Vector_index(w2, i) += patolette__Vector_index(w2, i - 1);
    }

    for (size_t i = 1; i < size; i++) {
        patolette__Matrix2D_index(w1, 0, i) += patolette__Matrix2D_index(w1, 0, i - 1);
        patolette__Matrix2D_index(w1, 1, i) += patolette__Matrix2D_index(w1, 1, i - 1);
        patolette__Matrix2D_index(w1, 2, i) += patolette__Matrix2D_index(w1, 2, i - 1);
    }

    for (size_t i = 1; i < size; i++) {
        for (size_t s = 0; s < 3; s++) {
            for (size_t r = 0; r <= s; r++) {
                double v = patolette__Matrix3D_index(wrs, r, s, i - 1);
                patolette__Matrix3D_index(wrs, r, s, i) += v;
            } 
        }
    }

    return cache;
}

double patolette__CELLS_get_cell_distortion(
    size_t a, 
    size_t b, 
    const patolette__CellMomentsCache *cache
) {
/*----------------------------------------------------------------------------
    Gets the distortion of a cell.

    @params
    a - The lower end of the cell.
    b - The upper end of the cell.
    cache - The CellMomentsCache object.
-----------------------------------------------------------------------------*/
    patolette__UInt64Array *w0 = cache->w0;
    patolette__Matrix2D *w1 = cache->w1;
    patolette__Vector *w2 = cache->w2;

    uint64_t w0a = patolette__UInt64Array_index(w0, a);
    uint64_t w0b = patolette__UInt64Array_index(w0, b);
    
    if (w0a == w0b) {
        return 0;
    }

    double w10a = patolette__Matrix2D_index(w1, 0, a);
    double w10b = patolette__Matrix2D_index(w1, 0, b);
    double w11a = patolette__Matrix2D_index(w1, 1, a);
    double w11b = patolette__Matrix2D_index(w1, 1, b);
    double w12a = patolette__Matrix2D_index(w1, 2, a);
    double w12b = patolette__Matrix2D_index(w1, 2, b);
    double w2a = patolette__Vector_index(w2, a);
    double w2b = patolette__Vector_index(w2, b);

    return (
        w2b - w2a -
        (
            SQ(w10b - w10a) +
            SQ(w11b - w11a) +
            SQ(w12b - w12a)
        ) / (double)(w0b - w0a)
    );
}

static double eval_vcov(
    size_t a, 
    size_t b,
    size_t r,
    size_t s,
    const patolette__CellMomentsCache *cache
) {
/*----------------------------------------------------------------------------
    Evaluates the variance-covariance matrix of a cell at a specific location.

    @params
    a - The lower end of the cell.
    b - The upper end of the cell.
    r - The row to evaluate.
    s - The column to evaluate.
    cache - The CellMomentsCache object.
-----------------------------------------------------------------------------*/
    patolette__UInt64Array *w0 = cache->w0;
    patolette__Matrix2D *w1 = cache->w1;
    patolette__Matrix3D *wrs = cache->wrs;

    uint64_t w0a = patolette__UInt64Array_index(w0, a);
    uint64_t w0b = patolette__UInt64Array_index(w0, b);
    
    if (w0a == w0b) {
        return 0;
    }

    double wrsa = patolette__Matrix3D_index(wrs, r, s, a);
    double wrsb = patolette__Matrix3D_index(wrs, r, s, b);
    double w1ra = patolette__Matrix2D_index(w1, r, a);
    double w1rb = patolette__Matrix2D_index(w1, r, b);
    double w1sa = patolette__Matrix2D_index(w1, s, a);
    double w1sb = patolette__Matrix2D_index(w1, s, b);

    return (
        (wrsb - wrsa) / (double)(w0b - w0a) -
        (w1rb - w1ra) * (w1sb - w1sa) / SQ((double)(w0b - w0a))
    );
}

static patolette__Matrix2D *patolette__CELLS_vcov(
    size_t a, 
    size_t b, 
    const patolette__CellMomentsCache *cache
) {
/*----------------------------------------------------------------------------
    Gets the variance-covariance matrix of a cell.

    @params
    a - The lower end of the cell.
    b - The upper end of the cell.
    cache - The CellMomentsCache object.
-----------------------------------------------------------------------------*/
    patolette__Matrix2D *vcov = patolette__Matrix2D_init(3, 3, NULL);
    for (size_t s = 0; s < 3; s++) {
        for (size_t r = 0; r <= s; r++) {
            double e = eval_vcov(a, b, r, s, cache);
            patolette__Matrix2D_index(vcov, r, s) = e;
        }
    }

    patolette__Matrix2D_index(vcov, 2, 0) = patolette__Matrix2D_index(vcov, 0, 2);
    patolette__Matrix2D_index(vcov, 1, 0) = patolette__Matrix2D_index(vcov, 0, 1);
    patolette__Matrix2D_index(vcov, 2, 1) = patolette__Matrix2D_index(vcov, 1, 2);
    return vcov;
}

/*----------------------------------------------------------------------------
    Internal functions END
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
    Exported functions START
-----------------------------------------------------------------------------*/

patolette__PCA *patolette__CELLS_perform_PCA(
    size_t a, 
    size_t b, 
    const patolette__CellMomentsCache *cache
) {
/*----------------------------------------------------------------------------
    Performs PCA on a cell.

    @params
    a - The lower end of the cell.
    b - The upper end of the cell.
    cache - The CellMomentsCache object.
-----------------------------------------------------------------------------*/
    patolette__Matrix2D *vcov = patolette__CELLS_vcov(a, b, cache);
    patolette__PCA *result = patolette__PCA_perform_PCA_vcov(vcov);
    patolette__Matrix2D_destroy(vcov);
    return result;
}

double patolette__CELLS_get_cell_bias(
    size_t a,
    size_t b,
    const patolette__Vector *axis,
    const patolette__CellMomentsCache *cache
) {
/*----------------------------------------------------------------------------
    Gets the bias of a cell towards a supplied axis.
    The cell's principal axis is angle-compared against the supplied axis.

    @params
    a - The lower end of the cell.
    b - The upper end of the cell.
    axis - The axis to compare to.
    cache - The CellMomentsCache object.

    @note
    If PCA fails, returns -1 as an error code.
-----------------------------------------------------------------------------*/

    patolette__PCA *pca = patolette__CELLS_perform_PCA(a, b, cache);
    if (pca == NULL) {
        return -1;
    }

    double result;

    double axis_norm = patolette__Vector_norm(axis);
    double cell_axis_norm = patolette__Vector_norm(pca->axis);
    double norms = axis_norm * cell_axis_norm;

    if (norms < patolette__DELTA) {
        result = 0;
    }

    else {
        double dot = (
            patolette__Vector_index(pca->axis, 0) * patolette__Vector_index(axis, 0) +
            patolette__Vector_index(pca->axis, 1) * patolette__Vector_index(axis, 1) +
            patolette__Vector_index(pca->axis, 2) * patolette__Vector_index(axis, 2)
        );

        double cos = dot / norms;
        result = fmin(1, fabs(cos));
    }

    patolette__PCA_destroy(pca);
    return result;
}

/*----------------------------------------------------------------------------
    Exported functions END
-----------------------------------------------------------------------------*/