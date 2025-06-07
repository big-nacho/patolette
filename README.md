***patolette*** is a **C / Python** color quantization and dithering library.

At its core, it implements a weighted variant of Xiaolin Wu's PCA-based quantizer (not to be confused with the popular one from _Graphics Gems vol. II_, which is already available [here](https://gist.github.com/bert/1192520)).

Some of its key features are:
- Avoids axis-aligned subdivisions.
- Supports the **CIEL\*u\*v\*** and **ICtCp** color spaces.
- Optional use of saliency maps to give higher priority to areas that stand out visually.
- Optional, blazing fast *KMeans* refinement.

The library is at its very early stages and in need of battle-testing and improvements, but it's already very usable.

## Installation
A **PyPI** package is not yet available. Until then, installation is manual, but it should hopefully be painless 🤞

If you do face any obstacles building / installing, please submit an issue! 🙏

 **Note for x86**\
*patolette* ships a slightly modified version of [faiss](https://github.com/facebookresearch/faiss) to aid with an optional *KMeans* refinement step. You can use the `pyproject.toml` file to specify an instruction set extension for it to be built with. If your CPU supports any of the **AVX** extensions, you can drastically increase *KMeans* performance.

The following will build the wheel and install it in the currently active virtual environment.

#### Linux (Debian)

```shell
# Clone repository
git clone https://github.com/big-nacho/patolette.git
cd patolette

# Install dependencies
apt install libopenblas-openmp-dev libflann-dev

# Build and install wheel
pip install .
```
Note on **OpenBLAS**: although any variant should in theory work, `libopenblas-openmp-dev` is recommended. If you have multiple variants installed, you may need to run the following before building for it to be linked properly.
```shell
sudo update-alternatives --set libblas.so.3-x86_64-linux-gnu /usr/lib/x86_64-linux-gnu/openblas-openmp/libblas.so.3
sudo update-alternatives --set liblapack.so.3-x86_64-linux-gnu /usr/lib/x86_64-linux-gnu/openblas-openmp/liblapack.so.3
```

#### macOS
```shell
# Clone repository
git clone https://github.com/big-nacho/patolette.git
cd patolette

# Install dependencies
brew install libomp flann

# Make sure system clang is used. If you use brew's clang 
# you may run into libstdc++ issues
export CC=/usr/bin/clang
export CXX=/usr/bin/clang++

# Let CMake find OpenMP
export OpenMP_ROOT=$(brew --prefix)/opt/libomp

# Build and install wheel
pip install .
```

## Basic Usage
The library doesn't take care of image decoding / encoding. You need to do that yourself. In the below example the [Pillow](https://pillow.readthedocs.io/en/stable/) library is used, but you can use whatever you want.
```python
import numpy as np
from PIL import Image
from patolette import quantize, ColorSpace_ICtCp

path = 'image.png'

# Read image
img = Image.open(path)
img = img.convert(mode = 'RGB')
width = img.width
height = img.height
img = np.asarray(img)

# Get colors (they should be in sRGB[0, 1] color space)
colors = img.reshape((-1, 3)).astype(np.float64)
colors /= 255

# Quantize
success, palette, palette_map, message = quantize(
    width,
    height,
    colors,
    256,
    dither=True,
    palette_only=False,
    color_space=ColorSpace_ICtCp,
    bias=1e-6,
    kmeans_niter=256,
    kmeans_max_samples=512 ** 2
)

if not success:
    print(message)
    exit()

# Palette is returned in sRGB[0, 1] color space
palette *= 255
palette = np.clip(palette, 0, 255)
palette = palette.astype(np.uint8)

# Save result
quantized = palette[palette_map].reshape(img.shape)
quantized = Image.fromarray(quantized)
quantized.save('result.png')
```

You can find more details on each parameter in the docstrings for the `quantize` function.

## Color Spaces
Three different color spaces are supported for the palette generation step. The following are rules of thumb you can go by, but experiment and see what works best for you:

**CIEL\*u\*v\***: generates exceptionally high quality color palettes and it's the best choice for very low color counts. However, it creates the least smooth results, and performs poorly on some hues.

**sRGB**: outputs relatively smooth results (and it's the most consistent in this regard) but it generates the lowest quality color palettes and it's not that well suited for lower color counts.

**ICtCp** (default): a good tradeoff between the two former. Generally, it generates slightly smoother results than **sRGB**, but it's a little bit less consistent at that, and the quality of the color palettes it generates is quite good.

## Bias
*patolette* optimizes against the sum of squared deviations when generating color palettes (the same way *KMeans* and other quantizers of similar nature do). This however comes with the known issue of large clusters dominating small, well defined ones. 

The *bias* parameter can be used to mitigate this issue. When non-zero, an extra step is introduced in the pipeline. A [saliency map](https://en.wikipedia.org/wiki/Saliency_map#:~:text=In%20computer%20vision%2C%20a%20saliency,an%20otherwise%20opaque%20ML%20model.) is computed and used to weight samples based on their visual importance. Below is a quick demo of non-biased (top-right) vs biased (bottom-left) quantization.

<p align="center">
  <img width="100%" src="https://github.com/user-attachments/assets/cf0cda53-947e-406d-ac9e-831c9a875899" />
</p>

Biased quantization can improve output quality significantly for images that contain sections that are relatively small but attention-grabbing. It can also enhance the quality of generated palettes for lower color counts.

## Caveats

### Memory Usage
One of the main priorities for `v1` is to decrease memory usage, but at the moment it is quite high. If you limit yourself to quantizing images up to **4k** resolution, you're on the very safe side, but if you go above **6k** you may start going into the danger zone depending on your system. Below is a chart depicting memory usage for different resolutions.

<p align="center">
  <img src="https://github.com/user-attachments/assets/7c2800cd-9334-431c-89aa-e29548346c0c" style="width:100%;"; />
</p>

### Using From C
The library can be called from C, but support for biased quantization via saliency maps is not there as of now. You can however supply your own weights if you want.
