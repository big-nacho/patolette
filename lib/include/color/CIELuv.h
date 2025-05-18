#pragma once

#include "array/matrix2D.h"

#include "math/misc.h"

#include "color/sRGB.h"

void patolette__COLOR_CIELuv_to_XYZ(
    double L,
    double u,
    double v,
    double *x,
    double *y,
    double *z
);
void patolette__COLOR_sRGB_Matrix_to_CIELuv_Matrix(patolette__Matrix2D *sRGB);
void patolette__COLOR_CIELuv_Matrix_to_Linear_Rec2020_Matrix(patolette__Matrix2D *Luv);