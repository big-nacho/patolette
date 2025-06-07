#pragma once

#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum patolette__ColorSpace {
    patolette__sRGB,
    patolette__CIELuv,
    patolette__ICtCp
} patolette__ColorSpace;

typedef struct patolette__QuantizationOptions {
    bool dither;
    bool palette_only;
    patolette__ColorSpace color_space;
    int kmeans_niter;
    size_t kmeans_max_samples;
} patolette__QuantizationOptions;

void patolette(
    size_t width,
    size_t height,
    const double *data,
    const double *weights,
    size_t palette_size,
    const patolette__QuantizationOptions *options,
    double *palette,
    size_t *palette_map,
    int *exit_code
);

const char *get_patolette_exit_code_info_message(int exit_code);
patolette__QuantizationOptions *patolette_create_default_options();