import numpy as np
from typing import Tuple, Optional

ColorSpace_CIELuv: int
ColorSpace_ICtCp: int
ColorSpace_sRGB: int

def quantize(
    width: int,
    height: int,
    colors: np.ndarray[Tuple[int, int], np.dtype[np.float64]],
    palette_size: int,
    dither: Optional[bool],
    palette_only: Optional[bool],
    color_space: Optional[int],
    tile_size: Optional[float],
    kmeans_niter: Optional[int],
    kmeans_max_samples: Optional[int],
    verbose: Optional[bool]
) -> int:
    """
    Quantizes color data.

    :param width:
        The width of the source image.
    :param height:
        The height of the source image.
    :param colors:
        A (width * height, 3) array containing the colors of the source image,
        scanned from left-to-right, top-to-bottom. Colors should be provided in *sRGB[0, 1]* space.
    :param palette_size:
        The desired palette size for the quantized image.
    :param dither:
        Whether dithering should be used. Default: *True*
    :param palette_only:
        When *True*, only a color palette is generated, and palette
        mapping is omitted. Default: *False*
    :param color_space:
        The color space to use for quantization. Only used for palette
        generation; dithering is always performed in Linear Rec2020,
        nearest neighbour mapping (when dithering is disabled) in ICtCp. Default: *ICtCp*
    :param tile_size:
            Tile size in the range [0, inf]. When tile_size > 0 is specified,
            a saliency map is computed, and visually striking areas are given
            higher weight. The lower the tile size, the more exaggerated the effect. Default: *512*
    :param kmeans_niter:
        Number of Kmeans refinment iterations to perform. Anything <= 0 yields no KMeans
        refinement. Default: *32*
    :param kmeans_max_samples:
        Maximum number of samples to use when performing KMeans refinement. There's a hard minimum
        of 256 ** 2. Default: *512 ** 2*
    :param verbose:
        Whether to print progress to console. Default: *false*
    :return out:
        - out[0]: Success flag.

        - out[1]: A (palette_size, 3) array describing the generated color palette in *sRGB[0, 1]* space.
            The array entries may not all be relveant, e.g *width* * *height* < *palette_size*. Non-relevant
            entries take the value of an out-of-range *sRGB* color, i.e [-1, -1, -1].

        - out[2]: A (width * height) array mapping each entry in *colors* to an entry in *out[1]*.
        
        - out[3]: A success / error message.
    """