#include "palette/nearest.h"

/*----------------------------------------------------------------------------
    This file defines functions to perform nearest neighbour queries via the
    FLANN library, all in the context of trying to find the closest color P
    in a color palette to some other color C.

    FLANN: https://github.com/flann-lib/flann
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
    Declarations START
-----------------------------------------------------------------------------*/

static double *build_index_data(
    const patolette__Matrix2D *colors,
    double fx,
    double fy,
    double fz
);

/*----------------------------------------------------------------------------
    Declarations END
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
    Internal functions START
-----------------------------------------------------------------------------*/

static double *build_index_data(
    const patolette__Matrix2D *colors,
    double fx,
    double fy,
    double fz
) {
/*----------------------------------------------------------------------------
    Builds a data array needed to create a FLANN index from a list of colors.

    @params
    colors - The list of colors.
    fx - A scale factor for the z coordinate of each color.
    fy - A scale factor for the y coordinate of each color.
    fz - A scale factor for the z coordinate of each color.

    @note
    Check dithering module for the reason behind the scale factors.
-----------------------------------------------------------------------------*/
    double *data = malloc(sizeof(double) * colors->rows * colors->cols);
    for (size_t i = 0; i < colors->rows; i++) {
        double cx = patolette__Matrix2D_index(colors, i, 0);
        double cy = patolette__Matrix2D_index(colors, i, 1);
        double cz = patolette__Matrix2D_index(colors, i, 2);
        data[i * 3] = cx * fx;
        data[i * 3 + 1] = cy * fy;
        data[i * 3 + 2] = cz * fz;
    }

    return data;
}

/*----------------------------------------------------------------------------
    Internal functions END
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
    Exported functions START
-----------------------------------------------------------------------------*/

flann_index_t patolette__PALETTE_build_palette_index(
    const patolette__Matrix2D *palette,
    double fx,
    double fy,
    double fz,
    struct FLANNParameters *params
) {
/*----------------------------------------------------------------------------
    Builds a FLANN index from a color palette that can be later used
    perform successive nearest neighbour queries.

    @params
    palette - The color palette.
    fx - A scale factor for the z coordinate of each color.
    fy - A scale factor for the y coordinate of each color.
    fz - A scale factor for the z coordinate of each color.
    params - FLANNParameters object.

    @note
    Check dithering module for the reason behind the scale factors.
-----------------------------------------------------------------------------*/
    size_t cols = 3;
    size_t rows = palette->rows;

    double *data = build_index_data(palette, fx, fy, fz);

    *params = DEFAULT_FLANN_PARAMETERS;
    params->algorithm = FLANN_INDEX_KDTREE_SINGLE;
    params->cores = 0;
    params->eps = 0;

    float speedup;
    flann_index_t flann_index = flann_build_index_double(
        data,
        (int)rows,
        (int)cols,
        &speedup,
        params
    );

    return flann_index;
}

size_t patolette__PALETTE_find_closest(
    double x,
    double y,
    double z,
    flann_index_t index,
    struct FLANNParameters *params
) {
/*----------------------------------------------------------------------------
    Finds the index of the closest color in a color palette to a supplied
    color.

    @params
    x - The x coordinate of the color.
    y - The y coordinate of the color.
    z - The z coordinate of the color.
    index - The color palette FLANN index.
    params - FLANNParameters object.
-----------------------------------------------------------------------------*/
    int i;
    double dist;
    double data[3] = { x, y, z };

    flann_find_nearest_neighbors_index_double(
        index,
        &data[0],
        1,
        &i,
        &dist,
        1,
        params
    );

    return (size_t)i;
}

void patolette__PALETTE_fill_palette_map_nearest(
    const patolette__Matrix2D *colors,
    const patolette__Matrix2D *palette,
    size_t *palette_map
) {
/*----------------------------------------------------------------------------
    Maps each color in a list to its closest palette color.

    @params
    colors - The list of colors.
    palette - The color palette.
    palette_map - The map to be filled.

    @note
    FLANN uses ints for sizes, so this function (assuming a non-potato system),
    will work on list of colors of up to 2147483647 items (unless there are
    more restrictions I'm not aware of, but it shouldn't be far off). That's
    roughly a 46340px * 46340px image. If anyone is ever crazy enough to want
    to quantize something that big, this could be updated to process things
    in chunks, or with a loop based approach (similar to how it's done for
    dithering).
-----------------------------------------------------------------------------*/
    size_t palette_rows = palette->rows;
    size_t colors_rows = colors->rows;

    double *palette_data = build_index_data(palette, 1, 1, 1);
    double *colors_data = build_index_data(colors, 1, 1, 1);

    int *indices = malloc(sizeof(int) * colors_rows);
    double *distances = malloc(sizeof(double) * colors_rows);

    struct FLANNParameters params = DEFAULT_FLANN_PARAMETERS;
    // Single KD-Tree is the fastest for our use case
    params.algorithm = FLANN_INDEX_KDTREE_SINGLE;
    // Use as many cores as available (although not sure if applies for KDTREE_SINGLE)
    params.cores = 0;
    // Exact nearest neighbour match
    params.eps = 0;

    flann_find_nearest_neighbors_double(
        palette_data,
        (int)palette_rows,
        3,
        colors_data,
        (int)colors_rows,
        indices,
        distances,
        1,
        &params
    );

    for (size_t i = 0; i < colors->rows; i++) {
        palette_map[i] = (size_t)(indices[i]);
    }

    free(palette_data);
    free(colors_data);
    free(indices);
    free(distances);
}

/*----------------------------------------------------------------------------
    Exported functions END
-----------------------------------------------------------------------------*/