#pragma once

#ifdef PATOLETTE_USE_ACCELERATE
#include<Accelerate/Accelerate.h>
#else
#include<cblas.h>
#include"math/lapack.h"
#endif

#ifdef PATOLETTE_USE_ACCELERATE
typedef int blasint;
#endif