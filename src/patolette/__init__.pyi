import numpy as np
from typing import Tuple, Optional

ColorSpace_CIELuv: int
ColorSpace_ICtCp: int

def quantize(
    width: int,
    height: int,
    colors: np.ndarray[Tuple[int, int], np.dtype[np.float64]],
    palette_size: int,
    dither: Optional[bool],
    palette_only: Optional[bool],
    color_space: Optional[ColorSpace_CIELuv, ColorSpace_ICtCp],
    bias: Optional[float],
    kmeans_niter: Optional[int],
    kmeans_max_samples: Optional[int]
) -> int:
    """
    Quantizes color data.

    Parameters
    ----------
        width : int
                The width of the source image.
        height : int
                The height of the source image.
        colors : ndarray[np.float64]
                A (width * height, 3) array containing the colors of the source image,
                scanned from left-to-right, top-to-bottom. Colors should be provided in *sRGB[0, 1]* space.
        palette_size : int
                The desired palette size for the quantized image.
        dither: bool
                Whether dithering should be used. Default: *True*
        palette_only: bool
                When *True*, only a color palette is generated, and palette
                mapping is omitted. Default: *False*
        color_space: [ColorSpace_CIELuv | ColorSpace_ICtCp | ColorSpace_sRGB]
                Color space to quantize in. Default: *ColorSpace_CIELuv*
        bias: float
            Bias amount in the range [0, 1]. When bias > 0 is specified,
            a saliency map is computed, and visually striking areas are given
            higher priority. The higher the bias, the more exaggerated the effect.
            The bias is a percentage of the number of pixels in the image.
        kmeans_niter: int
            Number of Kmeans refinment iterations to perform. Anything <= 0 yields no KMeans
            refinement.
        kmeans_max_samples: int
            Maximum number of samples to use when performing KMeans refinement.
    Returns
    ----------
        out : [bool, ndarray[np.float64] | None, ndarray[np.float64] | None, string]
            - out[0]: Success flag.
            - out[1]: A (palette_size, 3) array describing the generated color palette in *sRGB[0, 1]* space.
                The array entries may not all be relveant, e.g *width* * *height* < *palette_size*. Non-relevant
                entries take the value of an out-of-range *sRGB* color, i.e [-1, -1, -1].
            - out[2]: A (width * height) array mapping each entry in *colors* to an entry in *out[1]*.
            - out[3]: A success / error message.
    """