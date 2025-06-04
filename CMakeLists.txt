cmake_minimum_required(VERSION 3.27 FATAL_ERROR)

file(READ VERSION.txt PROJECT_VERSION)
project(patolette VERSION ${PROJECT_VERSION} LANGUAGES C CXX)

# One of "generic", "avx2", "avx512", "avx512_spr", "sve"
option(OPT_LEVEL "Optimization level" "generic")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(
  LIB_SOURCES_LIST

  lib/src/patolette.c

  lib/src/array/array.c
  lib/src/array/matrix2D.c
  lib/src/array/matrix3D.c
  lib/src/array/vector.c

  lib/src/color/CIELuv.c
  lib/src/color/rec2020.c
  lib/src/color/sRGB.c
  lib/src/color/ICtCp.c
  lib/src/color/xyz.c
  lib/src/color/eotf.c

  lib/src/dither/riemersma.c

  lib/src/math/eigen.c
  lib/src/math/pca.c

  lib/src/palette/create.c
  lib/src/palette/nearest.c
  lib/src/palette/refine.c

  lib/src/quantize/cells.c
  lib/src/quantize/cluster.c
  lib/src/quantize/global.c
  lib/src/quantize/local.c
  lib/src/quantize/sort.c
)

if (DEFINED SKBUILD)
  find_package(
    Python
    COMPONENTS Interpreter Development.Module NumPy
    REQUIRED
  )

  add_custom_command(
    OUTPUT patolette_cython.c
    COMMENT
      "Making ${CMAKE_CURRENT_BINARY_DIR}/patolette_cython.c from ${CMAKE_CURRENT_SOURCE_DIR}/patolette.pyx"
    COMMAND Python::Interpreter -m cython
            "${CMAKE_CURRENT_SOURCE_DIR}/src/patolette/patolette.pyx" --output-file patolette_cython.c
            --module-name patolette
    DEPENDS src/patolette/patolette.pyx
    VERBATIM
  )

  python_add_library(
    patolette MODULE
    patolette_cython.c
    ${LIB_SOURCES_LIST}
    WITH_SOABI
  )

  if (Python_NumPy_FOUND)
    target_compile_definitions(patolette PRIVATE NPY_NO_DEPRECATED_API)
    target_include_directories(patolette PRIVATE "${Python_NumPy_INCLUDE_DIRS}")
  else()
    message(FATAL_ERROR "Could not find NumPy headers")
  endif()

  install(TARGETS patolette DESTINATION patolette)
else()
  # UNTESTED
  add_library(
    patolette
    ${LIB_SOURCES_LIST}
  )

  set_target_properties(
    patolette
    PROPERTIES 
    VERSION ${PROJECT_VERSION}
    PUBLIC_HEADER lib/include/patolette.h
  )

  install(TARGETS patolette
    LIBRARY DESTINATION patolette/lib
    ARCHIVE DESTINATION patolette/lib
    PUBLIC_HEADER DESTINATION patolette/include
  )
endif()

if (APPLE)
  find_library(ACCELERATE_FRAMEWORK Accelerate REQUIRED)
  target_link_libraries(patolette PRIVATE ${ACCELERATE_FRAMEWORK})
  target_compile_definitions(patolette PRIVATE ACCELERATE_NEW_LAPACK)
  target_compile_definitions(patolette PRIVATE PATOLETTE_USE_ACCELERATE)
else()
  set(BLA_VENDOR "OpenBLAS")

  find_package(BLAS REQUIRED)
  target_link_libraries(patolette PRIVATE BLAS::BLAS)

  find_package(LAPACK REQUIRED)
  target_link_libraries(patolette PRIVATE LAPACK::LAPACK)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(FLANN REQUIRED flann)
target_link_libraries(patolette PRIVATE ${FLANN_LIBRARIES})
target_link_directories(patolette PRIVATE ${FLANN_LIBRARY_DIRS})
target_include_directories(patolette PRIVATE ${FLANN_INCLUDE_DIRS})

set(BUILD_SHARED_LIBS OFF)
add_subdirectory(lib/faiss)
target_link_libraries(patolette PRIVATE faiss_c)
target_include_directories(patolette PRIVATE lib)

target_include_directories(patolette PRIVATE lib/include)

target_compile_options(patolette PRIVATE -Wall)