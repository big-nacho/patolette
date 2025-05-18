#pragma once

#include "array/matrix2D.h"

#include "color/eotf.h"
#include "color/rec2020.h"
#include "color/sRGB.h"

#include "math/misc.h"

void patolette__COLOR_sRGB_Matrix_to_ICtCp_Matrix(patolette__Matrix2D *sRGB);