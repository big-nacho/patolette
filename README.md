***patolette*** is an image quantization and dithering library. 

It is, partly, an implementation of Xiaolin Wu's PCA-based quantizer (not to be confused with the popular one from *Graphics  Gems  vol. II*, which is already available [here](https://gist.github.com/bert/1192520)).

The library is a great choice for generating 256 color images (the output is exceptionally high quality), as well as quantizing to lower color counts.

On top of Wu's method, *patolette* adds support for *ICtCp* quantization, global K-means refinement, and an alternative optimization heuristic that aims to better deal with small, high variance clusters.

## Installation

*patolette* has only two dependencies that are easilly installable both on Linux and macOS. There are no binary distributions available at the time.

#### Linux (Debian)

```shell
# Install dependencies
apt install flann openblas

# Install patolette
pip install git+https://github.com/big-nacho/patolette
```

#### macOS

```shell
# Install dependencies
brew install flann

# Install dependencies (optional)
# If you're running macOS 10.3+ you can skip this, and patolette
# will try to build using Apple's Accelerate framework instead.
brew install openblas 

# Install patolette
pip install git+https://github.com/big-nacho/patolette
```
