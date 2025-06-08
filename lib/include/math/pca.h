#pragma once

#include<stddef.h>

#include "array/matrix2D.h"
#include "array/vector.h"

#include "math/eigen.h"
#include "math/linalg.h"

typedef struct patolette__PCA {
    patolette__Vector *axis;
    double explained_variance;
} patolette__PCA;

void patolette__PCA_destroy(patolette__PCA *pca);
patolette__PCA* patolette__PCA_perform_PCA_vcov(patolette__Matrix2D *vcov);
patolette__PCA *patolette__PCA_perform_PCA(
    const patolette__Matrix2D *m,
    const patolette__Vector *weights
);