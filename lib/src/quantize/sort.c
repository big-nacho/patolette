#include "quantize/sort.h"

/*----------------------------------------------------------------------------
   Bucketed axis-sorting of colors.
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
   Exported functions START
-----------------------------------------------------------------------------*/

patolette__IndexArray *patolette__SORT_axis_sort(
    const patolette__Matrix2D *colors,
    const patolette__Vector *axis,
    size_t bucket_count
) {
/*----------------------------------------------------------------------------
   Bucket sorts a list of colors based on their projection onto a supplied
   axis. Each bucket is not itself internally sorted.

   @param
   colors - The list of colors.
   axis - The axis to sort based on.
   bucket_count - The number of buckets to use.
-----------------------------------------------------------------------------*/
    size_t rows = colors->rows;
    size_t cols = colors->cols;

    patolette__IndexArray *map = patolette__IndexArray_init(rows);
    patolette__Vector *dots = patolette__Vector_init(rows);

    blasint m = (blasint)rows;
    blasint n = (blasint)cols;
    double alpha = 1;
    double *a = colors->data;
    blasint lda = m;
    double *x = axis->data;
    blasint incx = 1;
    double beta = 0;
    double *y = dots->data;
    blasint incy = 1;

    cblas_dgemv(
        CblasColMajor,
        CblasNoTrans,
        m,
        n,
        alpha,
        a,
        lda,
        x,
        incx,
        beta,
        y,
        incy
    );

    double min_dot = patolette__Vector_min(dots);
    double max_dot = patolette__Vector_max(dots);

    if (max_dot - min_dot < patolette__DELTA) {
        // In this case just assign buckets incrementally.
        // This might be very stupid. Maybe it's better to
        // switch between the first and last bucket, but
        // most likely it will never matter.
        size_t j = 0;
        for (size_t i = 0; i < rows; i++) {
            patolette__IndexArray_index(map, i) = j;
            if (j >= bucket_count - 1) {
                j = 0;
            }
            else {
                j++;
            }
        }

        patolette__Vector_destroy(dots);
        return map;
    }

    double s = 1 / (max_dot - min_dot);
    for (size_t i = 0; i < rows; i++) {
        double dot = patolette__Vector_index(dots, i);
        double ratio = (dot - min_dot) * s;
        size_t bucket = (size_t)((double)bucket_count * ratio);
        patolette__IndexArray_index(map, i) = min(bucket, bucket_count - 1);
    }

    patolette__Vector_destroy(dots);
    return map;
}

/*----------------------------------------------------------------------------
   Exported functions END
-----------------------------------------------------------------------------*/