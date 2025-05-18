#include "color/CIELuv.h"

/*----------------------------------------------------------------------------
   Conversions to CIELuv color space.

   CIELuv: https://en.wikipedia.org/wiki/CIELUV

   @note
   CIELuv -> XYZ is implemented here too to keep includes slightly
   simpler (I'm lazy).
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
   Constants START
-----------------------------------------------------------------------------*/

// Reference white for the D65 illuminant
static double rwx = 0.95047;
static double rwy = 1.0;
static double rwz = 1.08883;

static double kE = 216.0 / 24389.0;
static double kK = 24389.0 / 27.0;
static double kKE = 8.0;

/*----------------------------------------------------------------------------
   Constants END
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
   Declarations START
-----------------------------------------------------------------------------*/

static void XYZ_to_CIELuv(
    double x,
    double y,
    double z,
    double *L,
    double *u,
    double *v
);

/*----------------------------------------------------------------------------
   Declarations END
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
   Internal functions START
-----------------------------------------------------------------------------*/

static void XYZ_to_CIELuv(
    double x,
    double y,
    double z,
    double *L,
    double *u,
    double *v
) {
/*----------------------------------------------------------------------------
   Converts a color from CIE XYZ space to CIELuv space.

   @params
   x - X coordinate.
   y - Y coordinate.
   z - Z coordinate.
   L - output L coordinate.
   u - output u coordinate.
   v - output v coordinate.
-----------------------------------------------------------------------------*/
    double den = x + 15.0 * y + 3.0 * z;
    double up = (den > 0.0) ? ((4.0 * x) / (x + 15.0 * y + 3.0 * z)) : 0.0;
    double vp = (den > 0.0) ? ((9.0 * y) / (x + 15.0 * y + 3.0 * z)) : 0.0;

    double urp = (4.0 * rwx) / (rwx + 15.0 * rwy + 3.0 * rwz);
    double vrp = (9.0 * rwy) / (rwx + 15.0 * rwy + 3.0 * rwz);

    double yr = y / rwy;

    double L_ = (yr > kE) ? (116.0 * pow(yr, 1.0 / 3.0) - 16.0) : (kK * yr);
    double u_ = 13.0 * L_ * (up - urp);
    double v_ = 13.0 * L_ * (vp - vrp);

    *L = L_;
    *u = u_;
    *v = v_;
}

/*----------------------------------------------------------------------------
   Internal functions END
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
   Exported functions START
-----------------------------------------------------------------------------*/

void patolette__COLOR_CIELuv_to_XYZ(
    double L,
    double u,
    double v,
    double *x,
    double *y,
    double *z
) {
/*----------------------------------------------------------------------------
   Converts a color from CIELuv space to CIE XYZ space.

   @params
   L - L coordinate.
   u - u coordinate.
   v - v coordinate.
   x - output X coordinate.
   y - output Y coordinate.
   z - output Z coordinate.
-----------------------------------------------------------------------------*/
    double y_ = (L > kKE) ? pow((L + 16.0) / 116.0, 3.0) : (L / kK);
    double u0 = (4.0 * rwx) / (rwx + 15.0 * rwy + 3.0 * rwz);
    double v0 = (9.0 * rwy) / (rwx + 15.0 * rwy + 3.0 * rwz);

    double a;
    double a_den = u + 13.0 * L * u0;

    if (!a_den) {
        a = 0;
    }

    else {
        a = (((52.0 * L) / a_den) - 1.0) / 3.0;
    }

    double b = -5.0 * y_;
    double c = -1.0 / 3.0;

    double d;
    double d_den = v + 13.0 * L * v0;

    if (!d_den) {
        d = 0;
    }

    else {
        d = y_ * (((39.0 * L) / d_den) - 5.0);
    }

    double x_;
    double x_den = a - c;

    if (!x_den) {
        x_ = 0;
    }

    else {
        x_ = (d - b) / x_den;
    }

    double z_ = x_ * a + b;

    *x = x_;
    *y = y_;
    *z = z_;
}

void patolette__COLOR_sRGB_Matrix_to_CIELuv_Matrix(patolette__Matrix2D *sRGB) {
/*----------------------------------------------------------------------------
   Converts an sRGB color matrix to a CIELuv color matrix.
   The input matrix is modified.

   @params
   sRGB - The sRGB color matrix.
-----------------------------------------------------------------------------*/
    for (size_t i = 0; i < sRGB->rows; i++) {
        double r = patolette__Matrix2D_index(sRGB, i, 0);
        double g = patolette__Matrix2D_index(sRGB, i, 1);
        double b = patolette__Matrix2D_index(sRGB, i, 2);

        r = patolette__COLOR_sRGB_gamma_decode(r);
        g = patolette__COLOR_sRGB_gamma_decode(g);
        b = patolette__COLOR_sRGB_gamma_decode(b);

        double x = r * 0.4124564 + g * 0.3575761 + b * 0.1804375;
        double y = r * 0.2126729 + g * 0.7151522 + b * 0.0721750;
        double z = r * 0.0193339 + g * 0.1191920 + b * 0.9503041;

        double L;
        double u;
        double v;

        XYZ_to_CIELuv(x, y, z, &L, &u, &v);

        patolette__Matrix2D_index(sRGB, i, 0) = L;
        patolette__Matrix2D_index(sRGB, i, 1) = u;
        patolette__Matrix2D_index(sRGB, i, 2) = v;
    }
}

/*----------------------------------------------------------------------------
   Exported functions END
-----------------------------------------------------------------------------*/