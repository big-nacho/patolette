#pragma once

#include <stddef.h>
#include "flann/flann.h"

#include "array/matrix2D.h"
#include "array/matrix3D.h"
#include "array/vector.h"

#include "math/misc.h"

#include "palette/nearest.h"

void patolette__DITHER_riemersma(
    const patolette__Matrix2D *colors,
    size_t input_width,
    size_t input_height,
    patolette__Matrix2D *input_palette,
    size_t *input_palette_map
);