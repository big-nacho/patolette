#pragma once

#include "color/sRGB.h"

void patolette__COLOR_sRGB_to_XYZ(
    double r,
    double g,
    double b,
    double *x,
    double *y,
    double *z
);

void patolette__COLOR_Linear_Rec2020_to_XYZ(
    double r2020,
    double g2020,
    double b2020,
    double *x,
    double *y,
    double *z
);