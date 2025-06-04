# @lint-ignore-every LICENSELINT
# Copyright (c) Facebook, Inc. and its affiliates.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

# =============================================================================
# Copyright (c) 2023-2024, NVIDIA CORPORATION.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
# in compliance with the License. You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software distributed under the License
# is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
# or implied. See the License for the specific language governing permissions and limitations under
# the License.
# =============================================================================

cmake_minimum_required(VERSION 3.24.0 FATAL_ERROR)

set(FAISS_LANGUAGES CXX)

project(faiss
  VERSION 1.10.0
  DESCRIPTION "A library for efficient similarity search and clustering of dense vectors."
  HOMEPAGE_URL "https://github.com/facebookresearch/faiss"
  LANGUAGES ${FAISS_LANGUAGES})
include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Valid values are "generic", "avx2", "avx512", "avx512_spr", "sve".
option(FAISS_OPT_LEVEL "" "generic")
option(FAISS_ENABLE_GPU "Enable support for GPU indexes." OFF)
option(FAISS_ENABLE_CUVS "Enable cuVS for GPU indexes." OFF)
option(FAISS_ENABLE_ROCM "Enable ROCm for GPU indexes." OFF)
option(FAISS_ENABLE_MKL "Enable MKL." OFF)
option(FAISS_USE_LTO "Enable Link-Time optimization" OFF)

add_subdirectory(faiss)
add_subdirectory(c_api)

