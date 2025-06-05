***patolette*** is a **Python / C** color quantization and dithering library.

At its core, it implements a weighted variant of Xiaolin Wu's PCA-based quantizer (not to be confused with the popular one from _Graphics Gems vol. II_, which is already available [here](https://gist.github.com/bert/1192520)).

The library is at its very early stages and in need of battle-testing and improvements, but it's already very usable.

## Installation
A **PyPI** package (+binary distributions) is in the works. Until then, installation is manual, but it should hopefully be painless 🤞

If you do face any obstacles building / installing, please submit an issue! 🙏

 **Note for x86**\
*patolette* ships a slightly modified version of [faiss](https://github.com/facebookresearch/faiss) to aid with an optional *KMeans* refinement step. You can use the `pyproject.toml` file to specify an instruction set extension for it to be built with. If your CPU supports any of the **AVX** extensions, you can **drastically** increase *KMeans* performance.

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
*patolette* doesn't take care of image decoding / encoding. You need to do that yourself. In the below example the [Pillow](https://pillow.readthedocs.io/en/stable/) library is used, but you can use whatever you want.
```python
import numpy as np
from PIL import Image
from patolette import quantize, ColorSpace_ICtCp

path = 'path/to/image'

# Read image
img = Image.open(path)
img = img.convert(mode = 'RGB')
width = img.width
height = img.height
img = np.asarray(img)

# Get colors
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

palette *= 255
palette = np.clip(palette, 0, 255)
palette = palette.astype(np.uint8)

quantized = palette[palette_map].reshape(img.shape)
quantized = Image.fromarray(quantized)
quantized.save('result.png')
```

You can find more details on each parameter in the doc-strings for the `quantize` function.

## Color Spaces
Three different color spaces are supported (used for generating the quantized color palette). The following are rules of thumb you can go by, but experiment and see what works best for you:

**CIEL\*u\*v\***: generates exceptionally high quality color palettes, and it's the best choice for very low color counts. However, it creates the least smooth results, and performs poorly on some hues.

**sRGB**: outputs relatively smooth results (and it's the most consistent in this regard) but it generates the lowest quality color palettes, and it's not that well suited for lower color counts.

**ICtCp** (default): a good tradeoff between the two former. Generally, it generates slightly smoother results than **sRGB**, but it's a little bit less consistent at that, and the quality of the color palettes it generates is quite good.

## Bias
*patolette* optimizes against the sum of squared deviations when generating color palettes (the same way *KMeans* and other quantizers of similar nature do). This however comes with the known issue of large clusters dominating small, well defined ones. 

The *bias* parameter can be used to mitigate this issue. When non-zero, an extra step is introduced in the pipeline. A [saliency map](https://en.wikipedia.org/wiki/Saliency_map#:~:text=In%20computer%20vision%2C%20a%20saliency,an%20otherwise%20opaque%20ML%20model.) is computed and used to weight samples based on their visual importance. Below is a quick demo of non-biased (top-right) vs biased (bottom-left) quantization.

<p align="center">
  <img src="https://github.com/user-attachments/assets/ea50bad0-657f-4899-8371-fbe30df4b15d" />
</p>
