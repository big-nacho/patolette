#pragma once

#include "array/matrix2D.h"

#include "color/CIELuv.h"
#include "color/eotf.h"
#include "color/sRGB.h"
#include "color/xyz.h"

void patolette__COLOR_XYZ_to_Linear_Rec2020(
    double x,
    double y,
    double z,
    double *r2020,
    double *g2020,
    double *b2020
);

void patolette__COLOR_sRGB_to_Linear_Rec2020(
    double r,
    double g,
    double b,
    double *r2020,
    double *g2020,
    double *b2020
);

void patolette__COLOR_ICtCp_Matrix_to_Linear_Rec2020_Matrix(patolette__Matrix2D *ICtCp);
void patolette__COLOR_CIELuv_Matrix_to_Linear_Rec2020_Matrix(patolette__Matrix2D *Luv);
void patolette__COLOR_sRGB_Matrix_to_Linear_Rec2020_Matrix(patolette__Matrix2D *sRGB);