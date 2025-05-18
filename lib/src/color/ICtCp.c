#include "color/ICtCp.h"

/*----------------------------------------------------------------------------
   Conversions to ICtCp color space.

   ICtCp: https://professional.dolby.com/siteassets/pdfs/ictcp_dolbywhitepaper_v071.pdf
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
   Declarations START
-----------------------------------------------------------------------------*/

static void Linear_Rec2020_to_ICtCp(
    double r2020,
    double g2020,
    double b2020,
    double *I,
    double *Ct,
    double *Cp
);

static void sRGB_to_ICtCp(
    double r,
    double g,
    double b,
    double *I,
    double *Ct,
    double *Cp
);

/*----------------------------------------------------------------------------
   Declarations END
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
   Internal functions START
-----------------------------------------------------------------------------*/

static void Linear_Rec2020_to_ICtCp(
    double r2020,
    double g2020,
    double b2020,
    double *I,
    double *Ct,
    double *Cp
) {
/*----------------------------------------------------------------------------
   Converts a color from linear Rec2020 (RGB) space to ICtCp space.

   @params
   r2020 - R coordinate.
   g2020 - G coordinate.
   b2020 - B coordinate.
   I - output I coordinate.
   Ct - output Ct coordinate.
   Cp - output Cp coordinate.

   @note
   This function doesn't output a true ICtCp triplet. The Ct
   coordinate is halved so that color differences can be evaluated
   using Euclidean distances.
   See https://www.portrait.com/resource-center/ictcp-color-difference-metric/
-----------------------------------------------------------------------------*/
    double L = (r2020 * 1688 + g2020 * 2146 + b2020 * 262) / 4096;
    double M = (r2020 * 683 + g2020 * 2951 + b2020 * 462) / 4096;
    double S = (r2020 * 99 + g2020 * 309 + b2020 * 3688) / 4096;

    double L_ = patolette__COLOR_eotf_inverse_ST2084(L);
    double M_ = patolette__COLOR_eotf_inverse_ST2084(M);
    double S_ = patolette__COLOR_eotf_inverse_ST2084(S);

    *I = L_ * 0.5 + M_ * 0.5;
    *Ct = (L_ * 6610 - M_ * 13613 + S_ * 7003) / 4096;
    *Cp = (L_ * 17933 - M_ * 17390 - S_ * 543) / 4096;

    *Ct *= 0.5;
}

static void sRGB_to_ICtCp(
    double r,
    double g,
    double b,
    double *I,
    double *Ct,
    double *Cp
) {
/*----------------------------------------------------------------------------
   Converts a color from non-linear sRGB space to ICtCp space.

   @params
   r - R coordinate.
   g - G coordinate.
   b - B coordinate.
   I - output I coordinate.
   Ct - output Ct coordinate.
   Cp - output Cp coordinate.

   @note
   This function doesn't output a true ICtCp triplet. The Ct
   coordinate is halved so that color differences can be evaluated
   using Euclidean distances.
   See https://www.portrait.com/resource-center/ictcp-color-difference-metric/
-----------------------------------------------------------------------------*/
    double r2020, g2020, b2020;
    patolette__COLOR_sRGB_to_Linear_Rec2020(r, g, b, &r2020, &g2020, &b2020);
    Linear_Rec2020_to_ICtCp(r2020, g2020, b2020, I, Ct, Cp);
}

/*----------------------------------------------------------------------------
   Internal functions END
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
   Exported functions START
-----------------------------------------------------------------------------*/

void patolette__COLOR_sRGB_Matrix_to_ICtCp_Matrix(patolette__Matrix2D *sRGB) {
/*----------------------------------------------------------------------------
   Converts a non-linear sRGB color matrix to an ICtCp color matrix.
   The input matrix is modified.

   @params
   sRGB - The sRGB color matrix. It must be of shape (N, 3).

   @note
   The resulting colors are not true ICtCp triplets. The Ct
   coordinates are halved so that color differences can be evaluated
   using Euclidean distances.
   See https://www.portrait.com/resource-center/ictcp-color-difference-metric/
-----------------------------------------------------------------------------*/
    for (size_t i = 0; i < sRGB->rows; i++) {
        double r = patolette__Matrix2D_index(sRGB, i, 0);
        double g = patolette__Matrix2D_index(sRGB, i, 1);
        double b = patolette__Matrix2D_index(sRGB, i, 2);

        double I, Ct, Cp;
        sRGB_to_ICtCp(r, g, b, &I, &Ct, &Cp);

        patolette__Matrix2D_index(sRGB, i, 0) = I;
        patolette__Matrix2D_index(sRGB, i, 1) = Ct;
        patolette__Matrix2D_index(sRGB, i, 2) = Cp;
    }
}

/*----------------------------------------------------------------------------
   Exported functions END
-----------------------------------------------------------------------------*/