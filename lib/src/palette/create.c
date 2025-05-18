#include "palette/create.h"

/*----------------------------------------------------------------------------
    Color palette creation from color clusters.
-----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
    Exported functions START
-----------------------------------------------------------------------------*/

patolette__Matrix2D *patolette__PALETTE_create(const patolette__ColorClusterArray *clusters) {
/*----------------------------------------------------------------------------
    Creates a color palette from a list of color clusters.
    Each cluster is visited and its center is used as a color.

    @param
    clusters - The list of clusters.
-----------------------------------------------------------------------------*/
    patolette__Matrix2D *palette = patolette__Matrix2D_init(clusters->length, 3, NULL);

    for (size_t i = 0; i < clusters->length; i++) {
        patolette__ColorCluster *cluster = patolette__ColorClusterArray_index(clusters, i);
        const patolette__Vector *center = patolette__ColorCluster_get_center(cluster);
        double cx = patolette__Vector_index(center, 0);
        double cy = patolette__Vector_index(center, 1);
        double cz = patolette__Vector_index(center, 2);
        patolette__Matrix2D_index(palette, i, 0) = cx;
        patolette__Matrix2D_index(palette, i, 1) = cy;
        patolette__Matrix2D_index(palette, i, 2) = cz;
    }

    return palette;
}

/*----------------------------------------------------------------------------
    Exported functions END
-----------------------------------------------------------------------------*/