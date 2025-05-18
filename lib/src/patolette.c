#include "patolette.h"

#include "color/CIELuv.h"
#include "color/ICtCp.h"
#include "color/rec2020.h"
#include "color/sRGB.h"

#include "dither/riemersma.h"

#include "palette/create.h"
#include "palette/refine.h"

#include "quantize/local.h"
#include "quantize/global.h"

/*----------------------------------------------------------------------------
    patolette

    This file is the library's public interface.
-----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
    Constants START
-----------------------------------------------------------------------------*/

static const int success = 0;
static const int bad_quant = -1;
static const int bad_dims = -2;
static const int bad_palette_size = -3;
static const int bad_bias = -4;
static const int huge_dims = -5;

static const char *exit_code_info_messages[6] = {
    "Quantization successful.\0",
    "Internal quantization error.\0",
    "Image dimensions should be greater than 0.\0",
    "Palette size should be greater than 0.\0",
    "Bias should be in the range [0, 1].\0",
    "Image dimensions are too big.\0",
};

/*----------------------------------------------------------------------------
    Constants END
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
    Internal functions START
-----------------------------------------------------------------------------*/

static void validate_arguments(
    size_t width,
    size_t height,
    size_t palette_size,
    const patolette__QuantizationOptions *options,
    int *exit_code
);

/*----------------------------------------------------------------------------
    Internal functions START
-----------------------------------------------------------------------------*/

static void validate_arguments(
    size_t width,
    size_t height,
    size_t palette_size,
    const patolette__QuantizationOptions *options,
    int *exit_code
) {
/*----------------------------------------------------------------------------
    Performs some validations on quantization arguments.

    @params
    width - The width of the image.
    height - The height of the image.
    palette_size - The desired palette size.
    options - Quantization options.
    exit_code - On exit, zero if successful, non-zero otherwise.
-----------------------------------------------------------------------------*/
    *exit_code = 0;

    size_t px_count = width * height;
    double bias = options->bias;

    if (px_count == 0) {
        *exit_code = bad_dims;
        return;
    }

    if (palette_size < 1) {
        *exit_code = bad_palette_size;
        return;
    }

    if (bias < 0 || bias > 1) {
        *exit_code = bad_bias;
    }

    if (width * height > 40000 * 40000) {
        *exit_code = huge_dims;
    }
}

const char *get_patolette_exit_code_info_message(const int exit_code) {
/*----------------------------------------------------------------------------
    Gets a success / error message from an exit code.

    @params
    exit_code - The exit code.
-----------------------------------------------------------------------------*/
    return exit_code_info_messages[-1 * exit_code];
}

patolette__QuantizationOptions *patolette_create_default_options() {
/*----------------------------------------------------------------------------
    Creates default options for quantization.
-----------------------------------------------------------------------------*/
    patolette__QuantizationOptions *options = malloc(sizeof *options);
    options->dither = true;
    options->palette_only = false;
    options->color_space = patolette__CIELuv;
    options->heuristic = patolette__HeuristicPatolette;
    options->kmeans_niter = 8;
    options->kmeans_max_samples = SQ(512);
    options->bias = 0.005;
    return options;
}

/**
 * Quantizes an image.
 *
 * @param width The width of the image.
 * @param height The height of the image.
 * @param data A (width * height, 3) matrix containing the image colors,
 *             scanned from left-to-right, top-to-bottom in sRGB[0, 1] space. The matrix
 *             must be stored column-major, i.e all red values come first, followed by all
 *             green values, followed by all blue values.
 * @param palette_size The desired palette size, or the number of colors to
 *                     quantize the image to.
 * @param options Quantization options.
 *  - dither: Whether dithering is desired.
 *  - palette_only: When true, only the color palette is generated, and palette
 *                  mapping is omitted.
 *  - color_space: The color space to use for quantization. Only used for palette
 *                 generation; dithering is always performed in Linear Rec2020,
 *                 nearest neighbour mapping (when dithering is disabled) in ICtCp.
 *  - heuristic: Selection heuristic to use. It determines which cluster is to be split
 *               next during the quantization process. Wu is "purer" since it tries to
 *               minimize sums of squared deviations. Patolette adds a variance-based
 *               bias that helps with small, visually impactful areas that otherwise can
 *               end up being under-prioritized. The strength of the bias can be controlled
 *               via the "bias" parameter.
 *  - bias: Bias strength. It must be in the range [0, 1]. Doesn't have any effect
 *          if patolette__HeuristicWu is used as heuristic.
 *  - kmeans_niter: Number of KMeans refinement iterations to perform.
 *  - kmeans_max_samples: Maximum number of samples to use when performing KMeans refinement.
 * @param palette_map A previously allocated array of length width * height.
 *                    The palette map is written here.
 * @param palette A previously allocated (palette_size, 3) matrix.
 *                The generated color palette is written here. Colors are written in
 *                sRGB[0,1] space.
 *                Some entries in the palette may be irrelevant, e.g width * height < palette_size.
 *                Non relevant entries take the value of an out of range sRGB[0, 1] color, i.e [-1, -1, -1].
 *                The matrix is written column-major, i.e all red values come first, followed by all
 *                green values, followed by all blue values.
 * @param exit_code Exit code. Zero if successful, non-zero otherwise.
 */
void patolette(
    size_t width,
    size_t height,
    const double *data,
    size_t palette_size,
    const patolette__QuantizationOptions *options,
    double *palette,
    size_t *palette_map,
    int *exit_code
) {
    validate_arguments(
        width,
        height,
        palette_size,
        options,
        exit_code
    );

    if (*exit_code != 0) {
        return;
    }

    bool dither = options->dither;
    bool palette_only = options->palette_only;
    patolette__ColorSpace color_space = options->color_space;
    patolette__Heuristic heuristic = options->heuristic;
    double bias = options->bias;
    int kmeans_niter = options->kmeans_niter;
    size_t kmeans_max_samples = options->kmeans_max_samples;

    patolette__Matrix2D *colors = patolette__Matrix2D_init(
        width * height,
        3,
        data
    );

    if (color_space == patolette__CIELuv) {
        patolette__COLOR_sRGB_Matrix_to_CIELuv_Matrix(colors);
    }
    else {
        patolette__COLOR_sRGB_Matrix_to_ICtCp_Matrix(colors);
    }

    patolette__ColorClusterArray *clusters = patolette__GQ_quantize(
        colors,
        palette_size
    );

    if (clusters == NULL) {
        // Error
        *exit_code = bad_quant;
        patolette__Matrix2D_destroy(colors);
        return;
    }

    clusters = patolette__LQ_quantize(
        width,
        height,
        clusters,
        palette_size,
        heuristic,
        bias
    );

    if (clusters == NULL) {
        // Error
        *exit_code = bad_quant;
        patolette__Matrix2D_destroy(colors);
        return;
    }

    patolette__Matrix2D *palette_colors;
    if (kmeans_niter > 0) {
        palette_colors = patolette__PALETTE_get_refined_palette(
            colors,
            clusters,
            kmeans_niter,
            kmeans_max_samples
        );
    }

    else {
        palette_colors = patolette__PALETTE_create(clusters);
    }

    if (!palette_only) {
        if (dither) {

            if (color_space == patolette__CIELuv) {
                patolette__COLOR_CIELuv_Matrix_to_Linear_Rec2020_Matrix(colors);
                patolette__COLOR_CIELuv_Matrix_to_Linear_Rec2020_Matrix(palette_colors);
            }

            else {
                patolette__COLOR_ICtCp_Matrix_to_Linear_Rec2020_Matrix(colors);
                patolette__COLOR_ICtCp_Matrix_to_Linear_Rec2020_Matrix(palette_colors);
            }

            patolette__DITHER_riemersma(
                colors,
                width,
                height,
                palette_colors,
                palette_map
            );

            patolette__COLOR_Linear_Rec2020_Matrix_to_sRGB_Matrix(colors);
            patolette__COLOR_Linear_Rec2020_Matrix_to_sRGB_Matrix(palette_colors);
        }
        else {
            if (color_space == patolette__CIELuv) {
                // This is pretty ugly.
                // TODO: implement more direct conversions
                patolette__COLOR_CIELuv_Matrix_to_Linear_Rec2020_Matrix(colors);
                patolette__COLOR_CIELuv_Matrix_to_Linear_Rec2020_Matrix(palette_colors);
                patolette__COLOR_Linear_Rec2020_Matrix_to_sRGB_Matrix(colors);
                patolette__COLOR_Linear_Rec2020_Matrix_to_sRGB_Matrix(palette_colors);
                patolette__COLOR_sRGB_Matrix_to_ICtCp_Matrix(colors);
                patolette__COLOR_sRGB_Matrix_to_ICtCp_Matrix(palette_colors);
            }

            patolette__PALETTE_fill_palette_map_nearest(
                colors,
                palette_colors,
                palette_map
            );

            patolette__COLOR_ICtCp_Matrix_to_Linear_Rec2020_Matrix(palette_colors);
            patolette__COLOR_Linear_Rec2020_Matrix_to_sRGB_Matrix(palette_colors);
        }
    }

    // For unset entries in the palette
    for (size_t j = 0; j < palette_size * 3; j++) {
        palette[j] = -1.0;
    }

    for (size_t j = 0; j < palette_colors->cols; j++) {
        for (size_t i = 0; i < palette_colors->rows; i++) {
            palette[palette_size * j + i] = patolette__Matrix2D_index(palette_colors, i, j);
        }
    }

    *exit_code = success;

    patolette__Matrix2D_destroy(colors);
    patolette__Matrix2D_destroy(palette_colors);
}