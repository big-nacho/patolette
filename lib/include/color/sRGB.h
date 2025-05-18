#pragma once

#include "array/matrix2D.h"

#include "color/xyz.h"

#include "math/misc.h"

double patolette__COLOR_sRGB_gamma_decode(double component);
double patolette__COLOR_sRGB_gamma_encode(double component);
void patolette__COLOR_Linear_Rec2020_Matrix_to_sRGB_Matrix(patolette__Matrix2D *Rec2020);