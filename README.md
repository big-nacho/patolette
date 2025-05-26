***patolette*** is a **C / Python** color quantization and dithering library.

It is, partly, an implementation of Xiaolin Wu's PCA-based quantizer (not to be confused with the popular one from *Graphics  Gems  vol. II*, which is already available [here](https://gist.github.com/bert/1192520)).

On top of Wu's method, ***patolette***  adds support for quantization based on saliency maps, global k-means refinement, and the *ICtCp* color space.

The library's output is already exceptionally high quality, but it is still at an early stage and in need of battle-testing and improvements.

## Installation
A **PyPI** package (+binary distributions) is in the works. Until then, you'll need to take care of dependencies and install / build directly from this repository, but hopefully it should be painless 🤞

If you do face any obstacles building / installing, please submit an issue! 🙏

#### Linux (Debian)

```shell
# Install dependencies
apt install flann openblas

# Install patolette
pip install git+https://github.com/big-nacho/patolette
```

#### macOS
⚠️ **gcc** / **g++** compilers are recommended. I've managed to build the library using **clang**, but it has also given me headaches depending on the system, mainly due to **OpenMP** / **libc++** trickery on macOS.

```shell
# Install dependencies
brew install gcc libomp flann

# Use gcc / g++
export CC=/path/to/brew/gcc
export CXX=/path/to/brew/g++

# Let CMake find OpenBLAS
export CMAKE_PREFIX_PATH=$(brew --prefix)/opt/openblas

# Install patolette
pip install git+https://github.com/big-nacho/patolette
```
