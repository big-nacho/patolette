#include "color/xyz.h"

/*----------------------------------------------------------------------------
   Conversions to CIE XYZ color space.

   CIE XYZ: https://en.wikipedia.org/wiki/CIE_1931_color_space
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
   Exported functions START
-----------------------------------------------------------------------------*/

void patolette__COLOR_sRGB_to_XYZ(
    double r,
    double g,
    double b,
    double *x,
    double *y,
    double *z
) {
/*----------------------------------------------------------------------------
   Converts a color from sRGB color space to CIE XYZ color space.

   @params
   r - R coordinate.
   g - G coordinate.
   b - B coordinate.
   x - Output X coordinate.
   y - Output Y coordinate.
   z - Output Z coordinate.
-----------------------------------------------------------------------------*/
    double R = patolette__COLOR_sRGB_gamma_decode(r);
    double G = patolette__COLOR_sRGB_gamma_decode(g);
    double B = patolette__COLOR_sRGB_gamma_decode(b);

    *x = R * 0.4124564 + G * 0.3575761 + B * 0.1804375;
    *y = R * 0.2126729 + G * 0.7151522 + B * 0.0721750;
    *z = R * 0.0193339 + G * 0.1191920 + B * 0.9503041;
}

void patolette__COLOR_Linear_Rec2020_to_XYZ(
    double r2020,
    double g2020,
    double b2020,
    double *x,
    double *y,
    double *z
) {
/*----------------------------------------------------------------------------
   Converts a color from Linear Rec2020 (RGB) color space to CIE XYZ color space.

   @params
   r2020 - R coordinate.
   g2020 - G coordinate.
   b2020 - B coordinate.
   x - Output X coordinate.
   y - Output Y coordinate.
   z - Output Z coordinate.
-----------------------------------------------------------------------------*/
    *x = r2020 * 0.63695351 + g2020 * 0.14461919 + b2020 * 0.16885585;
    *y = r2020 * 0.26269834 + g2020 * 0.67800877 + b2020 * 0.0592929;
    *z = g2020 * 0.02807314 + b2020 * 1.06082723;
}

/*----------------------------------------------------------------------------
   Exported functions END
-----------------------------------------------------------------------------*/