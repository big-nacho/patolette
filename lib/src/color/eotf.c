#include "color/eotf.h"

/*----------------------------------------------------------------------------
   Implementation of the SMPTE ST 2084 electro-optical
   transfer function and its inverse.
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
   Constants START
-----------------------------------------------------------------------------*/

static double Lp = 10000;
static double m1 = 0.1593017578125;
static double m2 = 78.84375;
static double c1 = 0.8359375;
static double c2 = 18.8515625;
static double c3 = 18.6875;

/*----------------------------------------------------------------------------
   Constants END
-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
   Exported functions START
-----------------------------------------------------------------------------*/

double patolette__COLOR_eotf_ST2084(double component) {
/*----------------------------------------------------------------------------
   Calculates the SMPTE ST 2084 electro-optical transfer function.

   @params
   component - L / M / S component
-----------------------------------------------------------------------------*/
    double m1d = 1 / m1;
    double m2d = 1 / m2;
    double V_p = pow(component, m2d);
    double n = fmax(0, V_p - c1);
    double L = pow((n / (c2 - c3 * V_p)), m1d);
    return Lp * L;
}

double patolette__COLOR_eotf_inverse_ST2084(double component) {
/*----------------------------------------------------------------------------
   Calculates the inverse of the SMPTE ST 2084 electro-optical transfer
   function.

   @params
   component - L / M / S component
-----------------------------------------------------------------------------*/
    double y_ = pow(component / Lp, m1);
    return pow(
        (c1 + c2 * y_) / (1 + c3 * y_),
        m2
    );
}

/*----------------------------------------------------------------------------
   Exported functions END
-----------------------------------------------------------------------------*/

