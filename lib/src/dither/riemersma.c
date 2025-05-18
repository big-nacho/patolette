#include "dither/riemersma.h"

/*----------------------------------------------------------------------------
    Riemersma dithering: https://www.compuphase.com/riemer.htm
    Input colors are expected in linear Rec2020 (RGB) color space. Testing
    showed that dithering in this wider gamut produces more pleasant results
    than linear sRGB (to me, at least).

    The code here is mostly adapted from https://www.compuphase.com/riemer.c
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
    Constants START
-----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
    The following weights are used to calculate RGB color differences.
    They correspond to the square roots of the coefficients used by Rec2020 to
    calculate the Y (luminance) component for YCbCr.

    During the dithering process, many nearest neighbour queries must be made
    to find the closest palette color to some unknown color. To do that quickly,
    a FLANN index is built first with all the palette colors.

    When inserting a palette color P into the index, it's inserted as:
        P' = P[R] * R_weight + P[G] * G_weight + P[B] * B_weight

    And when making a nearest neighbour query for color C, instead we query:
        C' = C[R] * R_weight + C[G] * G_weight + C[B] * B_weight

    When calculating the Euclidean norm ||P' - C'|| all weights
    end up squared, which is why we use square roots. In the end this yields
    a "perceptual luminance" difference, which is what we need for dithering.
-----------------------------------------------------------------------------*/

// sqrt(0.2627)
static double R_weight = 0.51254268114958;
// sqrt(0.678)
static double G_weight = 0.8234075540095561;
// sqrt(0.0593)
static double B_weight = 0.2435159132377184;

/*----------------------------------------------------------------------------
    Constants END
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
    Declarations START
-----------------------------------------------------------------------------*/

// Directions for traversing the Hilbert curve
typedef enum Direction {
    NONE,
    UP,
    LEFT,
    RIGHT,
    DOWN
} Direction;

// Ratio between the weights of the first and last element in the error queue
static size_t QR = 16;

// Error queue size
static size_t Q = 16;

// X and Y coordinates of the current pixel being dithered.
static size_t x;
static size_t y;

// Width and height of the image
static size_t width; 
static size_t height;

// Error queue. Stores the last Q error vectors encountered
static patolette__Matrix2D *error_queue;

// Weights for each entry in the error queue
static patolette__Vector *weights;

// Image as a 3D matrix
static patolette__Matrix3D *image;

// Color palette
static patolette__Matrix2D *palette;

// Reference to the palette map
static size_t *palette_map;

// FLANN index (used for nearest neighbor search)
static flann_index_t flann_index;

// FLANN params (used for nearest neighbor search)
static struct FLANNParameters flann_params;

static int get_level();
static void move(Direction direction);
static void traverse_level(int level, Direction direction);
static void shift_error_queue();
static void dither_current_pixel();

static void destroy_state();
static void init_error_queue();
static void init_weights();
static void init_image(const patolette__Matrix2D *colors);
static void init_state(
    const patolette__Matrix2D *colors,
    size_t input_width,
    size_t input_height,
    patolette__Matrix2D *input_palette,
    size_t *input_palette_map
);

/*----------------------------------------------------------------------------
    Declarations END
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
    Internal functions START
-----------------------------------------------------------------------------*/

static int get_level() {
/*----------------------------------------------------------------------------
    Gets the level (order) of the Hilbert curve to generate.

    Hilbert curve: https://en.wikipedia.org/wiki/Hilbert_curve
-----------------------------------------------------------------------------*/
    int level = 0;

    size_t max = max(width, height);
    size_t value = max;
    while (value > 1) {
        value >>= 1;
        level++;
    }

    if ((1L << level) < max) {
        level++;
    }

    return level;
}

static void move(Direction direction) {
/*----------------------------------------------------------------------------
    Dithers the pixel at the current x, y position (if any), and moves
    one step in some direction.
-----------------------------------------------------------------------------*/
    if (
        x >= 0 && x < width &&
        y >= 0 && y < height
    ) {
        dither_current_pixel();
    }

    switch (direction) {
        case LEFT:
            x--;
            break;
        case RIGHT:
            x++;
            break;
        case UP:
            y--;
            break;
        case DOWN:
            y++;
            break;
        case NONE:
            break;
    }
}

static void traverse_level( // NOLINT(*-no-recursion)
    int level,
    Direction direction
) {
/*----------------------------------------------------------------------------
    Traverses a Hilbert curve, dithering each encountered pixel in the
    process.

    @params
    level - The level (order) of the curve.
    direction - The direction in which to traverse.
-----------------------------------------------------------------------------*/
    if (level == 1) {
        switch (direction) {
            case LEFT:
                move(RIGHT);
                move(DOWN);
                move(LEFT);
                break;
            case RIGHT:
                move(LEFT);
                move(UP);
                move(RIGHT);
                break;
            case UP:
                move(DOWN);
                move(RIGHT);
                move(UP);
                break;
            case DOWN:
                move(UP);
                move(LEFT);
                move(DOWN);
                break;
            case NONE:
                break;
        }
    }

    else {
        switch (direction) {
            case LEFT:
                traverse_level(level - 1, UP);
                move(RIGHT);
                traverse_level(level - 1, LEFT);
                move(DOWN);
                traverse_level(level - 1, LEFT);
                move(LEFT);
                traverse_level(level - 1, DOWN);
                break;
            case RIGHT:
                traverse_level(level - 1, DOWN);
                move(LEFT);
                traverse_level(level - 1, RIGHT);
                move(UP);
                traverse_level(level - 1, RIGHT);
                move(RIGHT);
                traverse_level(level - 1, UP);
                break;
            case UP:
                traverse_level(level - 1, LEFT);
                move(DOWN);
                traverse_level(level - 1, UP);
                move(RIGHT);
                traverse_level(level - 1, UP);
                move(UP);
                traverse_level(level - 1, RIGHT);
                break;
            case DOWN:
                traverse_level(level - 1, RIGHT);
                move(UP);
                traverse_level(level - 1, DOWN);
                move(LEFT);
                traverse_level(level - 1, DOWN);
                move(DOWN);
                traverse_level(level - 1, LEFT);
                break;
            case NONE:
                break;
        }
    }
}

static void shift_error_queue() {
/*----------------------------------------------------------------------------
    Shifts the error queue one place to the left.

    @note
    Could be written smarter by keeping a pivot instead of re-writing
    the whole thing. But I don't think it would bring much benefit
    (haven't checked though).
-----------------------------------------------------------------------------*/
    for (size_t i = 0; i < Q - 1; i++) {
        patolette__Matrix2D_index(error_queue, i, 0) = patolette__Matrix2D_index(error_queue, i + 1, 0);
        patolette__Matrix2D_index(error_queue, i, 1) = patolette__Matrix2D_index(error_queue, i + 1, 1);
        patolette__Matrix2D_index(error_queue, i, 2) = patolette__Matrix2D_index(error_queue, i + 1, 2);
    }
}

static void dither_current_pixel() {
/*----------------------------------------------------------------------------
    Dithers the current pixel.

    This function:
    1. Looks at the pixel P at location x, y
    2. Calculates an error vector V from the error queue.
    3. Finds the closest color CP in the palette to P + V
    4. Updates the palette map at the corresponding location to be CP.
    5. Shifts the error queue one place to the left.
    6. Updates the rightmost entry in the queue to be the difference
      between P and CP.
-----------------------------------------------------------------------------*/
    double error_R = 0;
    double error_G = 0;
    double error_B = 0;

    for (size_t i = 0; i < Q; i++) {
        double weight = patolette__Vector_index(weights, i);
        error_R += patolette__Matrix2D_index(error_queue, i, 0) * weight;
        error_G += patolette__Matrix2D_index(error_queue, i, 1) * weight;
        error_B += patolette__Matrix2D_index(error_queue, i, 2) * weight;
    }

    double R = patolette__Matrix3D_index(image, y, x, 0);
    double G = patolette__Matrix3D_index(image, y, x, 1);
    double B = patolette__Matrix3D_index(image, y, x, 2);

    /*----------------------------------------------------------------------------
        I've experimented with clamping here, but results were always slightly
        better without it.

        So, as it stands, corrected_R, corrected_G, corrected_B can go outside the
        range [0, 1].
    -----------------------------------------------------------------------------*/
    double corrected_R = R + error_R;
    double corrected_G = G + error_G;
    double corrected_B = B + error_B;

    size_t index = patolette__PALETTE_find_closest(
        R_weight * corrected_R,
        G_weight * corrected_G,
        B_weight * corrected_B,
        flann_index,
        &flann_params
    );

    corrected_R = patolette__Matrix2D_index(palette, index, 0);
    corrected_G = patolette__Matrix2D_index(palette, index, 1);
    corrected_B = patolette__Matrix2D_index(palette, index, 2);

    patolette__Matrix3D_index(image, y, x, 0) = corrected_R;
    patolette__Matrix3D_index(image, y, x, 1) = corrected_G;
    patolette__Matrix3D_index(image, y, x, 2) = corrected_B;

    palette_map[y * width + x] = index;

    shift_error_queue();

    double diff_R = R - corrected_R;
    double diff_G = G - corrected_G;
    double diff_B = B - corrected_B;

    patolette__Matrix2D_index(error_queue, Q - 1, 0) = diff_R;
    patolette__Matrix2D_index(error_queue, Q - 1, 1) = diff_G;
    patolette__Matrix2D_index(error_queue, Q - 1, 2) = diff_B;
}

static void destroy_state() {
/*----------------------------------------------------------------------------
    Destroys entire state.
-----------------------------------------------------------------------------*/
    patolette__Matrix2D_destroy(error_queue);
    patolette__Vector_destroy(weights);
    patolette__Matrix3D_destroy(image);
    flann_free_index_double(flann_index, &flann_params);
}

static void init_error_queue() {
/*----------------------------------------------------------------------------
    Initializes error queue (zero-initialized).
-----------------------------------------------------------------------------*/
    error_queue = patolette__Matrix2D_init(Q, 3, NULL);
}

static void init_weights() {
/*----------------------------------------------------------------------------
    Initializes error weights.
-----------------------------------------------------------------------------*/
    weights = patolette__Vector_init(Q);

    double m = exp(log((double)QR) / ((double)Q - 1));

    double v = 1;
    for (size_t i = 0; i < Q; i++) {
        patolette__Vector_index(weights, i) = v / (double)QR;
        v *= m;
    }
}

static void init_image(const patolette__Matrix2D *colors) {
/*----------------------------------------------------------------------------
    Initializes image as a 3D matrix.

    @note
    This uses a bunch of memory. At some point should be changed
    for smart indexing of the 2D color matrix instead.
-----------------------------------------------------------------------------*/
    image = patolette__Matrix3D_init(height, width, 3);
    for (size_t i = 0; i < height; i++) {
        for (size_t j = 0; j < width; j++) {
            size_t index = i * width + j;
            double R = patolette__Matrix2D_index(colors, index, 0);
            double G = patolette__Matrix2D_index(colors, index, 1);
            double B = patolette__Matrix2D_index(colors, index, 2);
            patolette__Matrix3D_index(image, i, j, 0) = R;
            patolette__Matrix3D_index(image, i, j, 1) = G;
            patolette__Matrix3D_index(image, i, j, 2) = B;
        }
    }
}

static void init_state(
    const patolette__Matrix2D *colors,
    size_t input_width,
    size_t input_height,
    patolette__Matrix2D *input_palette,
    size_t *input_palette_map
) {
/*----------------------------------------------------------------------------
    Initializes entire state.
-----------------------------------------------------------------------------*/
    x = 0;
    y = 0;

    width = input_width;
    height = input_height;
    palette = input_palette;
    palette_map = input_palette_map;

    init_error_queue();
    init_weights();
    init_image(colors);

    flann_index = patolette__PALETTE_build_palette_index(
        palette,
        (float)R_weight,
        (float)G_weight,
        (float)B_weight,
        &flann_params
    );
}

/*----------------------------------------------------------------------------
    Internal functions END
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
    Exported functions START
-----------------------------------------------------------------------------*/

void patolette__DITHER_riemersma(
    const patolette__Matrix2D *colors,
    size_t input_width, 
    size_t input_height,
    patolette__Matrix2D *input_palette,
    size_t *input_palette_map
) {
    init_state(
        colors,
        input_width,
        input_height,
        input_palette,
        input_palette_map
    );

    int level = get_level();
    if (level > 0) {
        traverse_level(level, UP);
        move(NONE);
    }

    destroy_state();
}

/*----------------------------------------------------------------------------
    Exported functions END
-----------------------------------------------------------------------------*/