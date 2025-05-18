import numpy as np
from types import SimpleNamespace

cimport cython
cimport numpy as cnp

cdef extern from 'patolette.h':
    cpdef enum patolette__ColorSpace:
        patolette__CIELuv
        patolette__ICtCp

    cpdef enum patolette__Heuristic:
        patolette__HeuristicWu
        patolette__HeuristicPatolette

    ctypedef struct patolette__QuantizationOptions:
        bint dither
        bint palette_only
        int kmeans_niter
        size_t kmeans_max_samples
        double bias
        patolette__ColorSpace color_space
        patolette__Heuristic heuristic

    void patolette(
        size_t width,
        size_t height,
        double *colors,
        size_t palette_size,
        patolette__QuantizationOptions *options,
        double *palette,
        size_t *palette_map,
        int *exit_code
    )

    const char *get_patolette_exit_code_info_message(int exit_code)

ColorSpace_CIELuv = patolette__ColorSpace.patolette__CIELuv
ColorSpace_ICtCp = patolette__ColorSpace.patolette__ICtCp
Heuristic_Wu = patolette__Heuristic.patolette__HeuristicWu
Heuristic_Patolette = patolette__Heuristic.patolette__HeuristicPatolette

color_mismatch = "The number of colors doesn't match the supplied width and height."
bad_channel_count = 'Expected colors to be in sRGB[0, 1] space. Channel count mismatch: {} found.'

def quantize(
    size_t width,
    size_t height,
    cnp.ndarray[cython.double, ndim = 2] colors,
    size_t palette_size,
    bint dither = True,
    bint palette_only = False,
    int kmeans_niter = 32,
    size_t kmeans_max_samples = 512 ** 2,
    double bias = 0.005,
    patolette__ColorSpace color_space = patolette__ColorSpace.patolette__CIELuv,
    patolette__Heuristic heuristic = patolette__Heuristic.patolette__HeuristicPatolette
):
    shape = colors.shape
    color_count = shape[0]
    channel_count = shape[1]

    if channel_count != 3:
        return (
            False,
            None,
            None,
            bad_channel_count.format(channel_count)
        )

    if color_count != width * height:
        return (
            False,
            None,
            None,
            color_mismatch
        )

    cdef patolette__QuantizationOptions opts
    opts.dither = dither
    opts.palette_only = palette_only
    opts.kmeans_niter = kmeans_niter
    opts.kmeans_max_samples = kmeans_max_samples
    opts.bias = bias
    opts.color_space = color_space
    opts.heuristic = heuristic

    cdef cython.double *data_pointer = cython.NULL
    cdef cython.double *palette_pointer = cython.NULL
    cdef cython.size_t *palette_map_pointer = cython.NULL

    cdef cython.double[::1, :] data = np.asfortranarray(
        colors, 
        dtype = np.double
    )

    if (data.shape[0] > 0):
        data_pointer = &data[0, 0]

    cdef cython.double[::1, :] palette = np.zeros(
        (palette_size, 3),
        dtype = np.double, 
        order = 'F'
    )

    if (palette.shape[0] > 0):
        palette_pointer = &palette[0, 0]

    cdef cython.size_t[::1] palette_map
    if not opts.palette_only:
        palette_map = np.zeros(
            width * height, 
            dtype = np.uintp, 
            order = 'F'
        )

        if (palette_map.shape[0] > 0):
            palette_map_pointer = &palette_map[0]

    cdef cython.int exit_code = 0

    patolette(
        <cython.size_t>width,
        <cython.size_t>height,
        <cython.double *>data_pointer,
        <cython.size_t>palette_size,
        <patolette__QuantizationOptions*>&opts,
        <cython.double *>palette_pointer,
        <cython.size_t *>palette_map_pointer,
        <cython.int *>&exit_code
    )

    success = exit_code == 0
    message = get_patolette_exit_code_info_message(exit_code)
    message = message.decode('UTF-8')

    if not success:
        return (
            success,
            None,
            None,
            message
        )

    if opts.palette_only:
        return (
            success,
            np.asarray(palette),
            None,
            message
        )

    return (
        success,
        np.asarray(palette),
        np.asarray(palette_map),
        message
    )

__all__ = [
    "quantize",
    "ColorSpace_CIELuv",
    "ColorSpace_ICtCp",
    "Heuristic_Wu",
    "Heuristic_Patolette"
]