import math
import numpy as np
import scipy.spatial.distance
import scipy.signal
import skimage
import skimage.io
from skimage.util import img_as_float
import cv2
import time

cimport cython
cimport numpy as cnp

'''----------------------------------------------------------------------------
   Declarations START
-----------------------------------------------------------------------------'''

cdef extern from 'patolette.h':
    cpdef enum patolette__ColorSpace:
        patolette__sRGB
        patolette__CIELuv
        patolette__ICtCp

    ctypedef struct patolette__QuantizationOptions:
        bint dither
        bint palette_only
        int kmeans_niter
        size_t kmeans_max_samples
        double bias
        patolette__ColorSpace color_space

    void patolette(
        size_t width,
        size_t height,
        double *color_data,
        double *weight_data,
        size_t palette_size,
        patolette__QuantizationOptions *options,
        double *palette,
        size_t *palette_map,
        int *exit_code
    )

    const char *get_patolette_exit_code_info_message(int exit_code)

'''----------------------------------------------------------------------------
   Declarations END
-----------------------------------------------------------------------------'''


'''----------------------------------------------------------------------------
   Saliency START

   NOTE: need to figure out how to separate cython things into multiple
   pyx / pxd files.
-----------------------------------------------------------------------------'''

def raster_scan(
    size_t rows,
    size_t cols,
    cython.float[:, :] img,
    cython.float[:, :] L,
    cython.float[:, :] U,
    cython.float[:, :] D
):
    cdef float ix
    cdef float d
    cdef float u1
    cdef float l1
    cdef float u2
    cdef float l2
    cdef float b1
    cdef float b2
    cdef size_t x = 1
    cdef size_t y = 1

    while x < rows - 1:
        y = 1
        while y < cols - 1:
            ix = img[x][y]
            d = D[x][y]

            u1 = U[x - 1][y]
            l1 = L[x - 1][y]

            u2 = U[x][y - 1]
            l2 = L[x][y - 1]

            b1 = max(u1, ix) - min(l1, ix)
            b2 = max(u2, ix) - min(l2, ix)

            if d <= b1 and d <= b2:
                y += 1
                continue
            elif b1 < d and b1 <= b2:
                D[x][y] = b1
                U[x][y] = max(u1, ix)
                L[x][y] = min(l1, ix)
            else:
                D[x][y] = b2
                U[x][y] = max(u2, ix)
                L[x][y] = min(l2, ix)

            y += 1

        x += 1

    return True

def raster_scan_inv(
    size_t rows,
    size_t cols,
    cython.float[:, :] img,
    cython.float[:, :] L,
    cython.float[:, :] U,
    cython.float[:, :] D
):
    cdef float ix
    cdef float d
    cdef float u1
    cdef float l1
    cdef float u2
    cdef float l2
    cdef float b1
    cdef float b2
    cdef size_t x = rows - 2
    cdef size_t y = cols - 2

    while x > 1:
        y = cols - 2
        while y > 1:
            ix = img[x][y]
            d = D[x][y]

            u1 = U[x+1][y]
            l1 = L[x+1][y]

            u2 = U[x][y+1]
            l2 = L[x][y+1]

            b1 = max(u1,ix) - min(l1,ix)
            b2 = max(u2,ix) - min(l2,ix)

            if d <= b1 and d <= b2:
                y -= 1
                continue
            elif b1 < d and b1 <= b2:
                D[x][y] = b1
                U[x][y] = max(u1,ix)
                L[x][y] = min(l1,ix)
            else:
                D[x][y] = b2
                U[x][y] = max(u2,ix)
                L[x][y] = min(l2,ix)

            y -= 1

        x -= 1

    return True

def mbd(cnp.ndarray[cython.float, ndim = 2] img, int iter):
    if (img.shape[0] <= 3 or img.shape[1] <= 3):
        return None

    cdef cnp.ndarray[cython.float, ndim = 2] L = np.array(img, copy = True)
    cdef cnp.ndarray[cython.float, ndim = 2] U = np.array(img, copy = True)
    cdef cnp.ndarray[cython.float, ndim = 2] D = (
        float('Inf') * 
        np.ones((img.shape[0], img.shape[1]))
    ).astype(np.float32)

    D[0, :] = 0
    D[-1, :] = 0
    D[:, 0] = 0
    D[:, -1] = 0

    cdef size_t rows = img.shape[0]
    cdef size_t cols = img.shape[1]
    cdef cython.float[:, :] img_view = img
    cdef cython.float[:, :] L_view = L
    cdef cython.float[:, :] U_view = U
    cdef cython.float[:, :] D_view = D

    for x in range(0, iter):
        if x % 2 == 1:
            raster_scan(
                rows,
                cols,
                img_view,
                L_view,
                U_view,
                D_view
            )
        else:
            raster_scan_inv(
                rows,
                cols,
                img_view,
                L_view,
                U_view,
                D_view
            )

    return D

def get_weights(cnp.ndarray[cython.double, ndim = 3] img, double bias):
    img_mean = np.mean(img, axis = 2).astype(np.float32)
    sal = mbd(img_mean, 3)

    cdef size_t rows = img.shape[0]
    cdef size_t cols = img.shape[1]

    cdef double img_size = math.sqrt(rows * cols)
    cdef int border_thickness = int(math.floor(0.1 * img_size))

    img_lab = img_as_float(skimage.color.rgb2lab(img))

    px_left = img_lab[0:border_thickness, :, :]
    px_right = img_lab[rows - border_thickness - 1: -1, :, :]

    px_top = img_lab[:, 0:border_thickness, :]
    px_bottom = img_lab[:, cols - border_thickness - 1 : -1, :]

    px_mean_left = np.mean(px_left, axis = (0, 1))
    px_mean_right = np.mean(px_right, axis = (0, 1))
    px_mean_top = np.mean(px_top, axis = (0, 1))
    px_mean_bottom = np.mean(px_bottom, axis = (0, 1))

    px_left = px_left.reshape((cols * border_thickness, 3))
    px_right = px_right.reshape((cols * border_thickness, 3))

    px_top = px_top.reshape((rows * border_thickness, 3))
    px_bottom = px_bottom.reshape((rows * border_thickness, 3))

    cov_left = np.cov(px_left.T)
    cov_right = np.cov(px_right.T)

    cov_top = np.cov(px_top.T)
    cov_bottom = np.cov(px_bottom.T)

    cov_left = np.linalg.inv(cov_left)
    cov_right = np.linalg.inv(cov_right)

    cov_top = np.linalg.inv(cov_top)
    cov_bottom = np.linalg.inv(cov_bottom)

    u_left = np.zeros((sal.shape[0], sal.shape[1]))
    u_right = np.zeros((sal.shape[0], sal.shape[1]))
    u_top = np.zeros((sal.shape[0], sal.shape[1]))
    u_bottom = np.zeros((sal.shape[0], sal.shape[1]))

    u_final = np.zeros((sal.shape[0], sal.shape[1]))
    img_lab_unrolled = img_lab.reshape(img_lab.shape[0] * img_lab.shape[1], 3)

    px_mean_left_2 = np.zeros((1, 3))
    px_mean_left_2[0, :] = px_mean_left

    u_left = scipy.spatial.distance.cdist(img_lab_unrolled,px_mean_left_2, 'mahalanobis', VI = cov_left)
    u_left = u_left.reshape((img_lab.shape[0],img_lab.shape[1]))

    px_mean_right_2 = np.zeros((1, 3))
    px_mean_right_2[0, :] = px_mean_right

    u_right = scipy.spatial.distance.cdist(img_lab_unrolled,px_mean_right_2, 'mahalanobis', VI = cov_right)
    u_right = u_right.reshape((img_lab.shape[0], img_lab.shape[1]))

    px_mean_top_2 = np.zeros((1, 3))
    px_mean_top_2[0, :] = px_mean_top

    u_top = scipy.spatial.distance.cdist(img_lab_unrolled,px_mean_top_2, 'mahalanobis', VI = cov_top)
    u_top = u_top.reshape((img_lab.shape[0], img_lab.shape[1]))

    px_mean_bottom_2 = np.zeros((1, 3))
    px_mean_bottom_2[0, :] = px_mean_bottom

    u_bottom = scipy.spatial.distance.cdist(img_lab_unrolled,px_mean_bottom_2, 'mahalanobis', VI = cov_bottom)
    u_bottom = u_bottom.reshape((img_lab.shape[0], img_lab.shape[1]))

    cdef float max_u_left = np.max(u_left)
    cdef float max_u_right = np.max(u_right)
    cdef float max_u_top = np.max(u_top)
    cdef float max_u_bottom = np.max(u_bottom)

    u_left = u_left / max_u_left
    u_right = u_right / max_u_right
    u_top = u_top / max_u_top
    u_bottom = u_bottom / max_u_bottom

    u_max = np.maximum(np.maximum(np.maximum(u_left,u_right),u_top),u_bottom)

    u_final = (u_left + u_right + u_top + u_bottom) - u_max

    cdef float u_max_final = np.max(u_final)
    cdef float sal_max = np.max(sal)

    sal = sal / sal_max + u_final / u_max_final
    sal = sal / np.max(sal)

    xv, yv = np.meshgrid(np.arange(sal.shape[1]),np.arange(sal.shape[0]))
    w = sal.shape[0]
    h = sal.shape[1]
    w2 = w / 2.0
    h2 = h / 2.0

    C = 1 - np.sqrt(np.power(xv - h2, 2) + np.power(yv - w2, 2)) / math.sqrt(np.power(w2, 2) + np.power(h2, 2))

    sal = sal * C

    def f(x):
        b = 10.0
        return 1.0 / (1.0 + math.exp(-b*(x - 0.5)))

    fv = np.vectorize(f)
    sal = sal / np.max(sal)
    sal = fv(sal)
    return 1 + np.reshape(sal, -1) * bias * rows * cols

'''----------------------------------------------------------------------------
   Saliency END
-----------------------------------------------------------------------------'''


'''----------------------------------------------------------------------------
   patolette START
-----------------------------------------------------------------------------'''

ColorSpace_sRGB = patolette__ColorSpace.patolette__sRGB
ColorSpace_CIELuv = patolette__ColorSpace.patolette__CIELuv
ColorSpace_ICtCp = patolette__ColorSpace.patolette__ICtCp

color_mismatch = "The number of colors doesn't match the supplied width and height."
bad_channel_count = 'Expected colors to be in sRGB[0, 1] space. Channel count mismatch: {} found.'
bad_bias = 'bias parameter expected to be in the range [0, 1]'

def quantize(
    size_t width,
    size_t height,
    cnp.ndarray[cython.double, ndim = 2] colors,
    size_t palette_size,
    bint dither = True,
    bint palette_only = False,
    patolette__ColorSpace color_space = patolette__ColorSpace.patolette__ICtCp,
    double bias = 5 * 1e-6,
    int kmeans_niter = 32,
    size_t kmeans_max_samples = 512 ** 2
):
    shape = colors.shape
    color_count = shape[0]
    channel_count = shape[1]

    # Some quick validations

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

    if bias < 0 or bias > 1:
        return (
            False,
            None,
            None,
            bad_bias
        )

    cdef patolette__QuantizationOptions opts
    opts.dither = dither
    opts.palette_only = palette_only
    opts.kmeans_niter = kmeans_niter
    opts.kmeans_max_samples = kmeans_max_samples
    opts.bias = bias
    opts.color_space = color_space

    cdef cython.double *color_data_pointer = cython.NULL
    cdef cython.double *weight_data_pointer = cython.NULL
    cdef cython.double *palette_pointer = cython.NULL
    cdef cython.size_t *palette_map_pointer = cython.NULL

    cdef cython.double[::1, :] data = np.asfortranarray(
        colors, 
        dtype = np.double
    )

    if (data.shape[0] > 0):
        color_data_pointer = &data[0, 0]

    cdef cython.double[::1, :] palette = np.zeros(
        (palette_size, 3),
        dtype = np.double, 
        order = 'F'
    )

    if (palette.shape[0] > 0):
        palette_pointer = &palette[0, 0]

    cdef cnp.ndarray[cython.double, ndim = 3] img
    cdef cython.double[::1] weights
    if (bias > 0):
        img = np.reshape(colors, (height, width, 3))
        weights = get_weights(img, bias)

        if (weights is not None):
            weight_data_pointer = &weights[0]

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
        <cython.double *>color_data_pointer,
        <cython.double *>weight_data_pointer,
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
    "ColorSpace_sRGB",
    "ColorSpace_CIELuv",
    "ColorSpace_ICtCp"
]

'''----------------------------------------------------------------------------
   patolette END
-----------------------------------------------------------------------------'''