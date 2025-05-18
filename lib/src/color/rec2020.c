#include "color/rec2020.h"

/*----------------------------------------------------------------------------
   Conversions to Rec2020 (RGB) color space.

   Rec2020: https://en.wikipedia.org/wiki/Rec._2020
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
   Declarations START
-----------------------------------------------------------------------------*/

static void ICtCp_to_Linear_Rec2020(
    double I,
    double Ct,
    double Cp,
    double *r2020,
    double *g2020,
    double *b2020
);

/*----------------------------------------------------------------------------
   Declarations END
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
   Internal functions START
-----------------------------------------------------------------------------*/

static void ICtCp_to_Linear_Rec2020(
    double I,
    double Ct,
    double Cp,
    double *r2020,
    double *g2020,
    double *b2020
) {
/*----------------------------------------------------------------------------
   Converts a color from ICtCp space to Linear Rec2020 (RGB) space.

   @params
   I - I coordinate.
   Ct - Ct coordinate.
   Cp - Cp coordinate.
   r2020 - Output R coordinate.
   g2020 - Output G coordinate.
   b2020 - Output B coordinate.

   @note
   Functions in ICtCp.c output ICtCp triplets with a halved Ct
   coordinate (check relevant functions descriptions), thus
   the Ct coordinate is first doubled here.
-----------------------------------------------------------------------------*/
    Ct *= 2;

    double L_ = I + 0.00860904 * Ct + 0.11102963 * Cp;
    double M_ = I - 0.00860904 * Ct - 0.11102963 * Cp;
    double S_ = I + 0.56003134 * Ct - 0.32062717 * Cp;

    double L = patolette__COLOR_eotf_ST2084(L_);
    double M = patolette__COLOR_eotf_ST2084(M_);
    double S = patolette__COLOR_eotf_ST2084(S_);

    *r2020 = L * 3.43660669 - M * 2.50645212 + S * 0.06984542;
    *g2020 = -L * 0.79132956 + M * 1.98360045 - S * 0.1922709;
    *b2020 = -L * 0.0259499 - M * 0.09891371 + S * 1.12486361;
}

/*----------------------------------------------------------------------------
   Internal functions END
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
   Exported functions START
-----------------------------------------------------------------------------*/

void patolette__COLOR_XYZ_to_Linear_Rec2020(
    double x,
    double y,
    double z,
    double *r2020,
    double *g2020,
    double *b2020
) {
/*----------------------------------------------------------------------------
   Converts a color from CIE XYZ space to Linear Rec2020 (RGB) space.

   @params
   x - X coordinate.
   y - Y coordinate.
   z - Z coordinate.
   r2020 - Output R coordinate.
   g2020 - Output G coordinate.
   b2020 - Output B coordinate.
-----------------------------------------------------------------------------*/
    *r2020 = x * 1.71666343 + y * -0.35567332 + z * -0.25336809;
    *g2020 = x * -0.66667384 + y * 1.61645574 + z * 0.0157683;
    *b2020 = x * 0.01764248 + y * -0.04277698 + z * 0.94224328;
}

void patolette__COLOR_sRGB_to_Linear_Rec2020(
    double r,
    double g,
    double b,
    double *r2020,
    double *g2020,
    double *b2020
) {
/*----------------------------------------------------------------------------
   Converts a color from sRGB space to Linear Rec2020 (RGB) space.

   @params
   r - R coordinate.
   g - G coordinate.
   b - B coordinate.
   r2020 - Output R coordinate.
   g2020 - Output G coordinate.
   b2020 - Output B coordinate.
-----------------------------------------------------------------------------*/
    double x, y, z;
    patolette__COLOR_sRGB_to_XYZ(r, g, b, &x, &y, &z);
    patolette__COLOR_XYZ_to_Linear_Rec2020(x, y, z, r2020, g2020, b2020);
}

void patolette__COLOR_ICtCp_Matrix_to_Linear_Rec2020_Matrix(patolette__Matrix2D *ICtCp) {
/*----------------------------------------------------------------------------
   Converts an ICtCp color matrix to a Linear Rec2020 (RGB) color matrix.
   The input matrix is modified.

   @params
   ICtCp - The ICtCp color matrix.
-----------------------------------------------------------------------------*/
    for (size_t i = 0; i < ICtCp->rows; i++) {
        double I = patolette__Matrix2D_index(ICtCp, i, 0);
        double Ct = patolette__Matrix2D_index(ICtCp, i, 1);
        double Cp = patolette__Matrix2D_index(ICtCp, i, 2);

        double r2020, g2020, b2020;
        ICtCp_to_Linear_Rec2020(I, Ct, Cp, &r2020, &g2020, &b2020);

        patolette__Matrix2D_index(ICtCp, i, 0) = r2020;
        patolette__Matrix2D_index(ICtCp, i, 1) = g2020;
        patolette__Matrix2D_index(ICtCp, i, 2) = b2020;
    }
}

void patolette__COLOR_CIELuv_Matrix_to_Linear_Rec2020_Matrix(patolette__Matrix2D *CIELuv) {
/*----------------------------------------------------------------------------
   Converts a CIELuv color matrix to a Linear Rec2020 (RGB) color matrix.
   The input matrix is modified.

   @params
   CIELuv - The CIELuv color matrix.
-----------------------------------------------------------------------------*/
    for (size_t i = 0; i < CIELuv->rows; i++) {
        double L = patolette__Matrix2D_index(CIELuv, i, 0);
        double u = patolette__Matrix2D_index(CIELuv, i, 1);
        double v = patolette__Matrix2D_index(CIELuv, i, 2);

        double x, y, z;
        patolette__COLOR_CIELuv_to_XYZ(L, u, v, &x, &y, &z);

        double r2020, g2020, b2020;
        patolette__COLOR_XYZ_to_Linear_Rec2020(x, y, z, &r2020, &g2020, &b2020);

        patolette__Matrix2D_index(CIELuv, i, 0) = r2020;
        patolette__Matrix2D_index(CIELuv, i, 1) = g2020;
        patolette__Matrix2D_index(CIELuv, i, 2) = b2020;
    }
}

/*----------------------------------------------------------------------------
   Exported functions END
-----------------------------------------------------------------------------*/