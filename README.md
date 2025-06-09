<p align="center">
    <img src="https://github.com/user-attachments/assets/c35f17b7-6c1f-499c-aa79-bbe9301fc6d2" width="150px" />
</p>
<p align="center">
    <img src="https://img.shields.io/badge/version-v0.0.1-blue" />
    <img src="https://img.shields.io/badge/beta-purple" />
</p>

***patolette*** is a **C / Python** color quantization and dithering library.

At its core, it implements a weighted variant of Xiaolin Wu's PCA-based quantizer (not to be confused with the popular one from _Graphics Gems vol. II_, which is already available [here](https://gist.github.com/bert/1192520)).

Some of its key features are:
- Avoids axis-aligned subdivisions.
- Supports the **CIEL\*u\*v\*** and **ICtCp** color spaces.
- Optional use of saliency maps to give higher weight to areas that stand out visually.
- Optional *KMeans* refinement.

The library is still in need of a ton of improvements and most definitely not ready for production use, but it's already very usable.

## Installation
A **PyPI** package is not yet available. Until then, installation is manual, but it should hopefully be painless 🤞

If you do face any obstacles building / installing, please submit an issue! 🙏

 ### Note for x86
*patolette* ships a slightly modified version of [faiss](https://github.com/facebookresearch/faiss) to aid with an optional *KMeans* refinement step. You can use the `CMAKE_ARGS` environment variable to specify an instruction set extension for it to be built with. If your CPU supports any of the **AVX** extensions, you can drastically increase *KMeans* performance.

For example, if your CPU supports **AVX512**
```shell
export CMAKE_ARGS="-DOPT_LEVEL=avx512"
```

The following will build the wheel and install it in the currently active virtual environment.

### Linux (Debian)

```shell
# Clone repository
git clone https://github.com/big-nacho/patolette.git
cd patolette

# Install dependencies
apt install libopenblas-openmp-dev libflann-dev

# Optional: set OPT_LEVEL (check Note for x86 section)
# Accepted values are "generic", "avx2", "avx512", "avx512_spr", "sve"
export CMAKE_ARGS="-DOPT_LEVEL=avx512"

# Build and install wheel
pip install .
```
Note on **OpenBLAS**: although any variant should in theory work, `libopenblas-openmp-dev` is recommended. If you have multiple variants installed, you may need to run the following before building for it to be linked properly.
```shell
sudo update-alternatives --set libblas.so.3-x86_64-linux-gnu /usr/lib/x86_64-linux-gnu/openblas-openmp/libblas.so.3
sudo update-alternatives --set liblapack.so.3-x86_64-linux-gnu /usr/lib/x86_64-linux-gnu/openblas-openmp/liblapack.so.3
```

### macOS
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

### Windows
Windows is of course a world of pain (but hey, no judgement if you're into that sort of thing ⛓️).

Small note: **MSVC** doesn't like building `faiss` with **AVX512**. Stick to **AVX2** if you're building with an instruction set extension on, at least until that's fixed. If you don't know what I'm talking about check [Note for x86](#note-for-x86).

The following may vary for you here and there, but mostly you should be able to build and install the wheel following these steps.

First, you need `pkg-config` or CMake won't find `flann`.
You can get it [here](https://sourceforge.net/projects/pkgconfiglite/files/) or you can install `pkgconfiglite` using [choco](https://chocolatey.org/) (that's what I did).

Then you need to get `flann` and `OpenBLAS`. You can do this in a variety of ways but an easy one is to use `conda`. You can get (Mini)conda [here](https://www.anaconda.com/docs/getting-started/miniconda/install).

With `conda` installed, open *Anaconda Prompt* and type

```bash
# Install dependencies
conda install conda-forge::openblas conda-forge::flann

# Get conda prefix
echo %CONDA_PREFIX%
```

`CONDA_PREFIX` will give you the prefix for your conda installation, keep it around.

Following that, this will build the wheel and place it inside a *dist* folder.

```powershell
# Clone repository
git clone https://github.com/big-nacho/patolette.git
cd patolette

# If CONDA_PREFIX = C:\miniconda3 then replace with C:\\miniconda3 (use double backslashes)
$env:CMAKE_ARGS = "-DCMAKE_PREFIX_PATH={CONDA_PREFIX}\\Library"

# Optional: set OPT_LEVEL (check Note for x86 section)
$env:CMAKE_ARGS = $env:CMAKE_ARGS + " " + "-DOPT_LEVEL=avx2"

# Install build module
pip install build

# Build wheel
python -m build
```

Now, you can't just install that wheel, because *.dll* files` won't be found at runtime. You need to repair it first. The following will repair and install the built wheel in your currently active virtual environment.

```powershell

# Install delvewheel
pip install delvewheel

# Repair wheel
delvewheel repair --add-path {CONDA_PREFIX}\\Library\\bin dist\\*.whl

# Install repaired wheel
pip install wheelhouse\\{WHEEL_NAME}.whl
```

## Basic Usage
The library doesn't take care of image decoding / encoding. You need to do that yourself. In the below example the [Pillow](https://pillow.readthedocs.io/en/stable/) library is used, but you can use whatever you want.

You can find more details on each parameter in the docstrings for the `quantize` function.
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
    # If you want progress console output
    verbose=True,
    # The following are all defaults
    dither=True,
    palette_only=False,
    color_space=ColorSpace_ICtCp,
    tile_size=512,
    kmeans_niter=32,
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
# NOTE: if you're trying to reduce file size,
# you may want to save a palette-based PNG instead
quantized = palette[palette_map].reshape(img.shape)
quantized = Image.fromarray(quantized)
quantized.save('result.png')
```

## Color Spaces
Three different color spaces are supported for the palette generation step. The following are rules of thumb you can go by, but experiment and see what works best for you:

**CIEL\*u\*v\***: generates exceptionally high quality color palettes and it's the best choice for very low color counts. However, it creates the least smooth results, and performs poorly on some hues.

**sRGB**: outputs relatively smooth results (and it's the most consistent in this regard) but it generates the lowest quality color palettes and it's not that well suited for lower color counts.

**ICtCp** (default): a good tradeoff between the two former. Generally, it generates slightly smoother results than **sRGB**, but it's a little bit less consistent at that, and the quality of the color palettes it generates is quite good.

## Tile Size
*patolette* optimizes against size-weighted variance during the palette generation stage (the same way *KMeans* and other quantizers of similar nature do). This however comes with the known issue of large clusters dominating small, well defined ones.

The `tile_size` parameter can be used to mitigate this issue. When non-zero, an extra step is introduced in the pipeline. A [saliency map](https://en.wikipedia.org/wiki/Saliency_map#:~:text=In%20computer%20vision%2C%20a%20saliency,an%20otherwise%20opaque%20ML%20model.) is computed and used to weight samples based on how much they stand out visually. The lower the tile size, the stronger the effect. The default tile size is `512`.

Below is a quick showcase.<br />

*top-left*: input image<br />
*top-right*: saliency map<br />
*bottom-left*: quantized with no saliency map<br />
*bottom-right*: quantized with saliency map.

<p align="center">
  <img width="100%" src="https://github.com/user-attachments/assets/8ad4784c-bfc6-4d0d-8130-acf8e5e4ebf2" />
</p>

## Caveats

### Memory Usage
The main priority for `v1` is to reduce memory consumption, at the moment it is quite high. If you limit yourself to quantizing images up to **4k** resolution you're on the pretty safe side, but if you go above **6k** you may start going into the danger zone depending on your system. Below is a chart depicting memory usage for different resolutions (*including* the space needed for the storage of the input image).

<p align="center">
  <img src="https://github.com/user-attachments/assets/7c2800cd-9334-431c-89aa-e29548346c0c" style="width:100%;" />
</p>

### Speed
It's "slow".

It's not nearly as fast as it could be yet, but will most likely stay slow compared to fast methods like median cut / octree, etc. Below is a chart with execution times for some resolutions, quantizing to 256 colors (ICtCp), *with* saliency maps and dithering on. Testing was performed on an 11 core Apple M3 Pro CPU.

<p align="center">
  <img src="https://github.com/user-attachments/assets/b34d55cc-3778-4f90-a688-f81e8c5c8077" style="width:100%;" />
</p>

### Using From C
Until `v1` is ready, the C API is incomplete. Mainly, support for weighted quantization via saliency maps is not there, so you won't find a `tile_size` parameter. It does however allow you to supply your own weights if you want.

### No RGBA support
For the time being, images with transparency are not supported, though if you have the pretty common use case of subject + fully transparent background, you can always fake it yourself by using a mask, but results may not be optimal.

## Acknowledgements
This library stands on the following works / projects.

*Color Quantization by Dynamic Programming and Principal Analysis, Xiaolin Wu* [[1]](https://dl.acm.org/doi/pdf/10.1145/146443.146475)

*Minimum Barrier Salient Object Detection at 80 FPS, Jianming Zhang, Stan Sclaroff, Zhe Lin, Xiaohui Shen, Brian Price, Randomír Mech* [[2]](https://openaccess.thecvf.com/content_iccv_2015/papers/Zhang_Minimum_Barrier_Salient_ICCV_2015_paper.pdf)

*Riemersma Dithering* [[3]](https://www.compuphase.com/riemer.htm)

[faiss](https://github.com/facebookresearch/faiss)

[flann](https://github.com/flann-lib/flann)

[OpenBLAS](https://github.com/OpenMathLib/OpenBLAS)

---
Thanks, Anna ❤️
