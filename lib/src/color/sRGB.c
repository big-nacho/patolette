#include "color/sRGB.h"

/*----------------------------------------------------------------------------
   Conversions to sRGB color space.

   sRGB: https://en.wikipedia.org/wiki/SRGB
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
   Declarations START
-----------------------------------------------------------------------------*/

static void Linear_Rec2020_to_sRGB(
    double r2020,
    double g2020,
    double b2020,
    double *r,
    double *g,
    double *b
);

/*----------------------------------------------------------------------------
   Declarations END
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
   Internal functions START
-----------------------------------------------------------------------------*/

void Linear_Rec2020_to_sRGB(
    double r2020,
    double g2020,
    double b2020,
    double *r,
    double *g,
    double *b
) {
/*----------------------------------------------------------------------------
   Converts a color from linear Rec2020 (RGB) space to sRGB space.

   @params
   r2020 - input R coordinate.
   g2020 - input G coordinate.
   b2020 - input B coordinate.
   r - output R coordinate.
   g - output G coordinate.
   b - output B coordinate.
-----------------------------------------------------------------------------*/
    double x, y, z;
    patolette__COLOR_Linear_Rec2020_to_XYZ(r2020, g2020, b2020, &x, &y, &z);
    *r = x * 3.2404542 - y * 1.5371385 - z * 0.4985314;
    *g = -x * 0.9692660 + y * 1.8760108 + z * 0.0415560;
    *b = x * 0.0556434 - y * 0.2040259 + z * 1.0572252;
    *r = patolette__COLOR_sRGB_gamma_encode(*r);
    *g = patolette__COLOR_sRGB_gamma_encode(*g);
    *b = patolette__COLOR_sRGB_gamma_encode(*b);
}

/*----------------------------------------------------------------------------
   Internal functions END
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
   Exported functions START
-----------------------------------------------------------------------------*/

double patolette__COLOR_sRGB_gamma_decode(double component) {
/*----------------------------------------------------------------------------
   Decodes gamma encoded sRGB component, i.e. evaluates
   the sRGB transfer function.

   @params
   component - sRGB component (R, G or B)
-----------------------------------------------------------------------------*/
    double result;

    if (component <= 0.0404500) {
        result = component / 12.92;
    }

    else {
        result = pow((component + 0.055) / 1.055, 2.4);
    }

    return fmin(fmax(result, 0.0), 1.0);
}

double patolette__COLOR_sRGB_gamma_encode(double component) {
/*----------------------------------------------------------------------------
   Gamma-encodes sRGB component, i.e. evaluates
   the sRGB inverse transfer function.

   @params
   component - linear sRGB component (R, G or B)
-----------------------------------------------------------------------------*/
    double result;

    if (component <= 0.0031308) {
        result = component * 12.92;
    }

    else {
        result = 1.055 * pow(component, 1.0 / 2.4) - 0.055;
    }

    return fmin(fmax(result, 0.0), 1.0);
}

void patolette__COLOR_Linear_Rec2020_Matrix_to_sRGB_Matrix(patolette__Matrix2D *Rec2020) {
/*----------------------------------------------------------------------------
   Converts a Linear Rec20200 color matrix to an sRGB color matrix.
   The input matrix is modified.

   @params
   Rec2020 - The linear Rec2020 color matrix. It must be of shape (N, 3).
-----------------------------------------------------------------------------*/
    for (size_t i = 0; i < Rec2020->rows; i++) {
        double r2020 = patolette__Matrix2D_index(Rec2020, i, 0);
        double g2020 = patolette__Matrix2D_index(Rec2020, i, 1);
        double b2020 = patolette__Matrix2D_index(Rec2020, i, 2);

        double r, g, b;
        Linear_Rec2020_to_sRGB(r2020, g2020, b2020, &r, &g, &b);

        patolette__Matrix2D_index(Rec2020, i, 0) = r;
        patolette__Matrix2D_index(Rec2020, i, 1) = g;
        patolette__Matrix2D_index(Rec2020, i, 2) = b;
    }
}

/*----------------------------------------------------------------------------
   Exported functions END
-----------------------------------------------------------------------------*/