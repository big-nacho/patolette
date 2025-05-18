import numpy as np
from typing import Tuple, Optional

ColorSpace_CIELuv: int
ColorSpace_ICtCp: int
Heuristic_Wu: int
Heuristic_Patolette: int

def quantize(
    width: int,
    height: int,
    colors: np.ndarray[Tuple[int, int], np.dtype[np.float64]],
    palette_size: int,
    dither: Optional[bool],
    palette_only: Optional[bool],
    kmeans_niter: Optional[int],
    color_space: Optional[ColorSpace_CIELuv, ColorSpace_ICtCp],
    heuristic: Optional[Heuristic_Wu, Heuristic_Patolette]
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
        options : QuantizationOptions, optional
                Quantization options.
                - dither: Whether dithering should be used. Default: *True*
                - palette_only: When *True*, only a color palette is generated, and palette
                mapping is omitted. Default: *False*
                - variance_stabilizer_factor: Controls the intensity of the variance stabilizer.
                                              Higher values can help quantize small, visually striking
                                              areas at the expense of overall quantization quality.
                - variance_tabilizer_kickoff: The quantization step at which the variance
                                              stabilizer starts to have an effect.
    Returns
    ----------
        out : [bool, ndarray[np.float64] | None, ndarray[np.float64] | None]
            - out[0]: Success flag.
            - out[1]: A (palette_size, 3) array describing the generated color palette in *sRGB[0, 1]* space.
                The array entries may not all be relveant, e.g *width* * *height* < *palette_size*. Non-relevant
                entries take the value of an out-of-range *sRGB* color, i.e [-1, -1, -1].
            - out[2]: A (width * height) array mapping each entry in *colors* to an entry in *out[1]*.
    """